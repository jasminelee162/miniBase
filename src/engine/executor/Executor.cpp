// src/engine/executor/executor.cpp
#include "executor.h"

#include <iostream>
#include <cstring>
#include <algorithm>
#include <sstream>
#include <cctype>
#include <functional>

#include "../../util/logger.h"
#include "../../storage/storage_engine.h"   // 使用 StorageEngine
#include "../../storage/index/bplus_tree.h" // 使用 BPlusTree
#include "../../storage/page/page.h"
#include "../operators/row.h"
#include "../../util/config.h"     // PAGE_SIZE, DEFAULT_MAX_PAGES (如果有)
#include "../../util/status.h"     // Status
#include "../../catalog/catalog.h" // Catalog

namespace minidb
{

    Logger Executor::logger("minidb.log");

    // 假设你有表的列名 schema，这里直接写死，也可以从 StorageEngine 获取
    static std::vector<std::string> table_schema = {"id", "name", "age"};

    // ----------------------
    // 从页内数据解析单列
    // ----------------------
    std::string Executor::parseColumnFromBuffer(const void *data, size_t &offset,
                                                const std::string &col_name,
                                                const std::string &table_name)
    {
        auto schema = catalog_->GetTable(table_name);
        int idx = schema.getColumnIndex(col_name);
        if (idx < 0)
            return "";

        const Column &col_schema = schema.columns[idx];
        const char *ptr = reinterpret_cast<const char *>(data);

        std::string val;
        if (col_schema.type == "INT")
        {
            int32_t num = 0;
            std::memcpy(&num, ptr + offset, sizeof(int32_t));
            val = std::to_string(num);
            offset += sizeof(int32_t);
        }
        else if (col_schema.type == "DOUBLE")
        {
            double num = 0.0;
            std::memcpy(&num, ptr + offset, sizeof(double));
            val = std::to_string(num);
            offset += sizeof(double);
        }
        else
        { // VARCHAR / CHAR / fallback
            size_t len = (col_schema.length > 0) ? col_schema.length : 64;
            val.assign(ptr + offset, len);
            val = val.c_str(); // 去掉可能的尾部 '\0'
            offset += len;
        }
        return val;
    }

    // ----------------------
    // 单页解析所有列
    // ----------------------
    Row Executor::parseRowFromPage(Page *page, const std::vector<std::string> &columns, const std::string &table_name)
    {
        Row row;
        if (!page)
            return row;

        char *data = page->GetData();
        size_t offset = 0;

        int32_t col_count = 0;
        std::memcpy(&col_count, data + offset, sizeof(int32_t));
        offset += sizeof(int32_t);

        for (auto &col_name : columns)
        {
            ColumnValue cv;
            cv.col_name = col_name;
            cv.value = parseColumnFromBuffer(data, offset, col_name, table_name); // 传入 table_name
            row.columns.push_back(cv);
        }

        return row;
    }

    /**
     * Helper: trim trailing '\0' and whitespace from string read from fixed-length field
     */
    static std::string trim_field(const char *src, size_t len)
    {
        std::string s(src, len);
        size_t pos = s.find('\0');
        if (pos != std::string::npos)
            s.resize(pos);
        while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r' || s.back() == '\n'))
            s.pop_back();
        return s;
    }

    // 判断 Row 是否匹配 predicate
    bool matchesPredicate(const Row &row, const std::string &predicate)
    {
        if (predicate.empty())
            return true;

        // 去掉空格
        std::string trimmed = predicate;
        trimmed.erase(std::remove_if(trimmed.begin(), trimmed.end(), isspace), trimmed.end());

        // 支持 =
        auto pos_eq = trimmed.find('=');
        if (pos_eq != std::string::npos)
        {
            std::string col = trimmed.substr(0, pos_eq);
            std::string val = trimmed.substr(pos_eq + 1);
            return row.getValue(col) == val;
        }

        // 支持 >
        auto pos_gt = trimmed.find('>');
        if (pos_gt != std::string::npos)
        {
            std::string col = trimmed.substr(0, pos_gt);
            std::string val = trimmed.substr(pos_gt + 1);
            try
            {
                int row_val = std::stoi(row.getValue(col));
                int cmp_val = std::stoi(val);
                return row_val > cmp_val;
            }
            catch (...)
            {
                return false;
            }
        }

        // 支持 <
        auto pos_lt = trimmed.find('<');
        if (pos_lt != std::string::npos)
        {
            std::string col = trimmed.substr(0, pos_lt);
            std::string val = trimmed.substr(pos_lt + 1);
            try
            {
                int row_val = std::stoi(row.getValue(col));
                int cmp_val = std::stoi(val);
                return row_val < cmp_val;
            }
            catch (...)
            {
                return false;
            }
        }

        return false; // 默认不匹配
    }

    // Helper: trim both ends
    static std::string trim(const std::string &s)
    {
        size_t a = 0, b = s.size();
        while (a < b && std::isspace((unsigned char)s[a]))
            ++a;
        while (b > a && std::isspace((unsigned char)s[b - 1]))
            --b;
        return s.substr(a, b - a);
    }

    // Try to interpret predicate like "colX > 10" or "col1 = Alice".
    // If that fails, fallback to substring matching in any column.
    static bool row_matches_predicate(const Row &row, const std::string &predicate)
    {
        std::string pred = trim(predicate);
        if (pred.empty())
            return true; // empty predicate matches all

        std::istringstream iss(pred);
        std::string left, op, right;
        if ((iss >> left) && (iss >> op) && (iss >> right))
        {
            if (right.size() >= 2 && ((right.front() == '"' && right.back() == '"') || (right.front() == '\'' && right.back() == '\'')))
            {
                right = right.substr(1, right.size() - 2);
            }

            // find column by name
            std::string left_trimmed = trim(left);
            int col_idx = -1;
            for (size_t i = 0; i < row.columns.size(); ++i)
            {
                if (row.columns[i].col_name == left_trimmed)
                {
                    col_idx = static_cast<int>(i);
                    break;
                }
            }

            if (col_idx == -1)
            {
                bool all_digits = !left_trimmed.empty() && std::all_of(left_trimmed.begin(), left_trimmed.end(), ::isdigit);
                if (all_digits)
                {
                    int idx = std::stoi(left_trimmed);
                    if (idx >= 0 && static_cast<size_t>(idx) < row.columns.size())
                        col_idx = idx;
                }
            }

            if (col_idx != -1)
            {
                const std::string &colval = row.columns[col_idx].value;
                bool left_is_num = !colval.empty() && (std::all_of(colval.begin(), colval.end(), [](char c)
                                                                   { return std::isdigit((unsigned char)c) || c == '-' || c == '+'; }));
                bool right_is_num = !right.empty() && (std::all_of(right.begin(), right.end(), [](char c)
                                                                   { return std::isdigit((unsigned char)c) || c == '-' || c == '+'; }));
                try
                {
                    if ((op == ">" || op == "<" || op == ">=" || op == "<=") && left_is_num && right_is_num)
                    {
                        long long L = std::stoll(colval);
                        long long R = std::stoll(right);
                        if (op == ">")
                            return L > R;
                        if (op == "<")
                            return L < R;
                        if (op == ">=")
                            return L >= R;
                        if (op == "<=")
                            return L <= R;
                    }
                    else if (op == "=" || op == "==")
                    {
                        if (left_is_num && right_is_num)
                            return std::stoll(colval) == std::stoll(right);
                        else
                            return colval == right;
                    }
                }
                catch (...)
                {
                    // fall through to substring fallback
                }
            }
        }

        // fallback: substring match any column
        for (const auto &c : row.columns)
        {
            if (c.value.find(predicate) != std::string::npos)
                return true;
        }
        return false;
    }

    int getColumnIndex(const minidb::TableSchema &schema, const std::string &col_name)
    {
        for (size_t i = 0; i < schema.columns.size(); i++)
        {
            if (schema.columns[i].name == col_name)
                return static_cast<int>(i);
        }
        return -1;
    }

    void Executor::execute(PlanNode *node)
    {
        if (!node)
        {
            std::cerr << "[Executor] Null PlanNode" << std::endl;
            return;
        }

        switch (node->type)
        {

        case PlanType::CreateTable:
        {
            logger.log("CREATE TABLE " + node->table_name);
            std::cout << "[Executor] 创建表: " << node->table_name << std::endl;

            if (catalog_)
            {
                // 直接使用 PlanNode 保存的完整列信息
                catalog_->CreateTable(node->table_name, node->table_columns);
            }
            break;
        }

        case PlanType::Insert:
        {
            logger.log("INSERT INTO " + node->table_name);
            std::cout << "[Executor] 插入到表: " << node->table_name << std::endl;

            if (!storage_engine_)
            {
                std::cerr << "[Executor] StorageEngine 未初始化！" << std::endl;
                break;
            }
            if (!catalog_ || !catalog_->HasTable(node->table_name))
            {
                std::cerr << "[Executor] Table not found in catalog: " << node->table_name << std::endl;
                break;
            }

            // 获取表 schema 和首页
            TableSchema schema = catalog_->GetTable(node->table_name);
            page_id_t first_page_id = schema.first_page_id;

            // 如果表还没有首页，则创建一个数据页并写回 schema
            Page *cur_page = nullptr;
            if (first_page_id == INVALID_PAGE_ID)
            {
                page_id_t new_pid = INVALID_PAGE_ID;
                cur_page = storage_engine_->CreateDataPage(&new_pid);
                if (!cur_page)
                {
                    std::cerr << "[Executor] 无法创建表数据页" << std::endl;
                    break;
                }
                first_page_id = new_pid;
                // 更新 catalog 内存，再持久化到页0（稍后统一保存）
                schema.first_page_id = first_page_id;
                // update internal map (catalog_ holds its own copy)
                // 如果你的 Catalog::GetTable 返回的是副本，需要一个 SetTable 或直接操作内部 map；这里假设有方法 SaveToStorage 会覆盖
                catalog_->CreateTable(schema.table_name, schema.columns); // 如果 CreateTable 已处理重复则改成直接更新内部 map
            }
            else
            {
                cur_page = storage_engine_->GetDataPage(first_page_id);
                if (!cur_page)
                {
                    // 首页丢失或被覆盖，尝试重新创建并设置
                    page_id_t new_pid = INVALID_PAGE_ID;
                    cur_page = storage_engine_->CreateDataPage(&new_pid);
                    if (!cur_page)
                    {
                        std::cerr << "[Executor] 无法获得或创建首页" << std::endl;
                        break;
                    }
                    first_page_id = new_pid;
                    schema.first_page_id = first_page_id;
                    catalog_->CreateTable(schema.table_name, schema.columns); // 同上，确保 catalog 内存更新
                }
            }

            // 为每条插入记录保存它实际写入的 page id（对应 node->values 顺序）
            std::vector<page_id_t> inserted_pids;
            inserted_pids.reserve(node->values.size());

            for (size_t row_idx = 0; row_idx < node->values.size(); ++row_idx)
            {
                // build Row from PlanNode row values
                Row row;
                for (size_t i = 0; i < node->columns.size(); ++i)
                {
                    ColumnValue cv;
                    cv.col_name = node->columns[i];
                    cv.value = node->values[row_idx][i];
                    row.columns.push_back(cv);
                }

                // 将 Row 序列化为字节（你需要实现 Row::Serialize 或等价函数）
                std::vector<char> buf;
                row.Serialize(buf, schema); // 必须实现：按 schema 将 row 转为 bytes（连续记录），返回 buf.size()

                // 尝试追加到当前页
                bool appended = storage_engine_->AppendRecordToPage(cur_page, buf.data(), static_cast<uint16_t>(buf.size()));
                if (!appended)
                {
                    // 当前页满：分配新页，链接，然后写入新页
                    page_id_t new_pid = INVALID_PAGE_ID;
                    Page *new_page = storage_engine_->CreateDataPage(&new_pid);
                    if (!new_page)
                    {
                        std::cerr << "[Executor] 无法分配新数据页" << std::endl;
                        break;
                    }
                    // 链接旧页 -> 新页
                    storage_engine_->LinkPages(cur_page->GetPageId(), new_pid);
                    // 记得 unpin 旧页（如果需要），但 AppendRecordToPage/PutPage 负责
                    cur_page = new_page;

                    // 再次尝试追加（若仍失败说明记录超大）
                    appended = storage_engine_->AppendRecordToPage(cur_page, buf.data(), static_cast<uint16_t>(buf.size()));
                    if (!appended)
                    {
                        std::cerr << "[Executor] 单条记录太大，无法写入空页，跳过或报错" << std::endl;
                        // 选择：跳过这条或中止整个插入；这里中止
                        break;
                    }
                }

                // 记录该行写入的 page id（slot 暂定 0）
                inserted_pids.push_back(cur_page->GetPageId());

                // 将当前页 unpin（不要标脏这里——AppendRecordToPage 可能已设置脏）
                storage_engine_->PutPage(cur_page->GetPageId(), true);
                // 将 cur_page 重新 fetch 回来以便后续追加（可选优化）
                cur_page = storage_engine_->GetDataPage(cur_page->GetPageId());
            }

            // 更新内存的 TableSchema（如果需要）
            // 假设 catalog_ 内部存的是副本，确保写回 first_page_id
            // 推荐提供 Catalog::UpdateTableFirstPageId() 或 SaveToStorage 会序列化当前内存结构
            catalog_->SaveToStorage(storage_engine_.get()); // 写回页0的Catalog元数据（含首页信息）

            // ========== 更新索引（逐条对应 inserted_pids） ==========
            std::vector<IndexSchema> indexes = catalog_->GetTableIndexes(node->table_name);
            for (auto &index : indexes)
            {
                if (index.type != "BPLUS")
                    continue;

                int col_idx = schema.getColumnIndex(index.cols[0]);
                if (col_idx < 0)
                    continue;

                BPlusTree bpt(storage_engine_.get());
                if (index.root_page_id == INVALID_PAGE_ID)
                {
                    index.root_page_id = bpt.CreateNew();
                }
                else
                {
                    bpt.SetRoot(index.root_page_id);
                }

                // 用每条插入记录对应的 page id 构造 RID，插入索引
                for (size_t i = 0; i < inserted_pids.size() && i < node->values.size(); ++i)
                {
                    const std::string &key_str = node->values[i][col_idx];
                    int32_t key = std::stoi(key_str);
                    RID rid{inserted_pids[i], 0}; // slot=0 暂定
                    bpt.Insert(key, rid);
                }

                // 可能根页发生变化（分裂），需要更新 index.root_page_id
                index.root_page_id = bpt.GetRoot();
            }

            // 最后把 catalog（包含更新后的 index.root_page_id）写回页0
            catalog_->SaveToStorage(storage_engine_.get());

            break;
        }

        case PlanType::SeqScan:
        {
            logger.log("SELECT * FROM " + node->table_name);
            std::cout << "[Executor] 查询表: " << node->table_name << std::endl;

            auto rows = SeqScanAll(node->table_name);
            std::cout << "[SeqScan] 读取到 " << rows.size() << " 行:" << std::endl;
            for (auto &row : rows)
                std::cout << "[Row] " << row.toString() << std::endl;
            break;
        }

        case PlanType::Delete:
        {
            logger.log("DELETE FROM " + node->table_name + " WHERE " + node->predicate);
            std::cout << "[Executor] 删除表: " << node->table_name
                      << " WHERE " << node->predicate << std::endl;

            if (!storage_engine_)
                break;

            size_t num_pages = storage_engine_->GetNumPages();
            size_t deleted = 0;

            for (page_id_t pid = 1; pid < static_cast<page_id_t>(num_pages); ++pid)
            {
                Page *p = storage_engine_->GetDataPage(pid); // 校验是数据页
                if (!p)
                    continue;

                std::vector<std::string> columns;
                if (catalog_ && catalog_->HasTable(node->table_name))
                    columns = catalog_->GetTableColumns(node->table_name);

                auto records = storage_engine_->GetPageRecords(p);
                std::vector<std::pair<const void *, uint16_t>> new_records;

                const minidb::TableSchema &schema = catalog_->GetTable(node->table_name);
                for (auto &rec : records)
                {
                    auto row = Row::Deserialize(reinterpret_cast<const unsigned char *>(rec.first), rec.second, schema);
                    if (!matchesPredicate(row, node->predicate))
                    {
                        new_records.push_back(rec);
                    }
                    else
                    {
                        ++deleted;
                    }
                }

                if (deleted > 0)
                {
                    p->InitializePage(PageType::DATA_PAGE); // 清空页
                    for (auto &rec : new_records)
                    {
                        storage_engine_->AppendRecordToPage(p, rec.first, rec.second);
                    }
                    storage_engine_->PutPage(pid, true);
                }
                else
                {
                    storage_engine_->PutPage(pid, false);
                }
            }

            std::cout << "[Delete] 共删除 " << deleted << " 行" << std::endl;
            break;
        }

            // Filter
        case PlanType::Filter:
        {
            logger.log("FILTER on " + node->predicate);
            std::cout << "[Executor] 过滤条件: " << node->predicate << std::endl;

            if (!catalog_ || !catalog_->HasTable(node->table_name))
                break;

            const auto &schema = catalog_->GetTable(node->table_name);
            std::vector<Row> filtered;

            // 遍历表的页链
            auto pages = storage_engine_->GetPageChain(schema.first_page_id);
            for (auto *p : pages)
            {
                auto records = storage_engine_->GetPageRecords(p);
                for (auto &rec : records)
                {
                    auto row = Row::Deserialize(reinterpret_cast<const unsigned char *>(rec.first),
                                                rec.second, schema);
                    if (matchesPredicate(row, node->predicate))
                        filtered.push_back(row);
                }
            }

            std::cout << "[Filter] 过滤后 " << filtered.size() << " 行:" << std::endl;
            for (auto &row : filtered)
                std::cout << "[Row] " << row.toString() << std::endl;

            break;
        }

        // Project
        case PlanType::Project:
        {
            logger.log("PROJECT columns");
            std::cout << "[Executor] 投影列: ";
            for (auto &col : node->columns)
                std::cout << col << " ";
            std::cout << std::endl;

            if (!catalog_ || !catalog_->HasTable(node->table_name))
                break;

            const auto &schema = catalog_->GetTable(node->table_name);
            std::vector<Row> projected;

            auto pages = storage_engine_->GetPageChain(schema.first_page_id);
            for (auto *p : pages)
            {
                auto records = storage_engine_->GetPageRecords(p);
                for (auto &rec : records)
                {
                    auto row = Row::Deserialize(reinterpret_cast<const unsigned char *>(rec.first),
                                                rec.second, schema);

                    Row r;
                    for (auto &col_name : node->columns)
                    {
                        int idx = schema.getColumnIndex(col_name);
                        if (idx >= 0 && idx < row.columns.size())
                            r.columns.push_back(row.columns[idx]);
                    }
                    projected.push_back(r);
                }
            }

            std::cout << "[Project] 投影后 " << projected.size() << " 行:" << std::endl;
            for (auto &row : projected)
                std::cout << "[Row] " << row.toString() << std::endl;

            break;
        }

        case PlanType::Update:
            Update(*node);
            break;

        default:
            std::cerr << "[Executor] 未知 PlanNode 类型" << std::endl;
        }
    }

    // ----------------------
    // 单页扫描
    std::vector<Row> Executor::SeqScan(Page *page, const minidb::TableSchema &schema)
    {
        std::vector<Row> result;
        if (!page)
            return result;

        auto records = storage_engine_->GetPageRecords(page);
        for (auto &rec : records)
        {
            Row row = Row::Deserialize(reinterpret_cast<const unsigned char *>(rec.first), rec.second, schema);
            if (!row.columns.empty())
                result.push_back(row);
        }

        return result;
    }

    // 遍历整张表
    std::vector<Row> Executor::SeqScanAll(const std::string &table_name)
    {
        std::vector<Row> all_rows;
        if (!storage_engine_ || !catalog_)
            return all_rows;

        const auto &schema = catalog_->GetTable(table_name);
        page_id_t first_page_id = schema.first_page_id;
        auto pages = storage_engine_->GetPageChain(first_page_id);

        for (auto p : pages)
        {
            auto rows = SeqScan(p, schema);
            all_rows.insert(all_rows.end(), rows.begin(), rows.end());
            storage_engine_->PutPage(p->GetPageId(), false);
        }

        return all_rows;
    }

    void Executor::Update(const PlanNode &plan)
    {
        std::cout << "[Executor] 更新表: " << plan.table_name
                  << " WHERE " << plan.predicate << std::endl;

        if (!catalog_ || !catalog_->HasTable(plan.table_name))
        {
            std::cerr << "[Update] 表不存在或 Catalog 未初始化" << std::endl;
            return;
        }

        if (!storage_engine_)
        {
            std::cerr << "[Update] StorageEngine 未初始化！" << std::endl;
            return;
        }

        const auto &schema = catalog_->GetTable(plan.table_name);
        size_t num_pages = storage_engine_->GetNumPages();
        int updated_count = 0;

        for (page_id_t pid = 1; pid < static_cast<page_id_t>(num_pages); ++pid)
        {
            Page *p = storage_engine_->GetDataPage(pid);
            if (!p)
                continue;

            auto records = storage_engine_->GetPageRecords(p);

            // ✅ 用 vector<char> 存真正的数据，避免悬空指针
            std::vector<std::vector<char>> new_records;

            bool page_modified = false;

            for (auto &rec : records)
            {
                auto row = Row::Deserialize(reinterpret_cast<const unsigned char *>(rec.first),
                                            rec.second, schema);

                if (matchesPredicate(row, plan.predicate))
                {
                    // 更新字段
                    for (const auto &kv : plan.set_values)
                        row.setValue(kv.first, kv.second);

                    // 重新序列化
                    std::vector<char> buf;
                    row.Serialize(buf, schema);
                    new_records.push_back(std::move(buf));

                    page_modified = true;
                    ++updated_count;
                }
                else
                {
                    // 保留原始记录
                    new_records.emplace_back(
                        reinterpret_cast<const char *>(rec.first),
                        reinterpret_cast<const char *>(rec.first) + rec.second);
                }
            }

            if (page_modified)
            {
                // 清空页并写入更新后的记录
                p->InitializePage(PageType::DATA_PAGE);
                for (auto &rec : new_records)
                    storage_engine_->AppendRecordToPage(p, rec.data(), rec.size());

                storage_engine_->PutPage(pid, true);
            }
            else
            {
                storage_engine_->PutPage(pid, false);
            }
        }

        std::cout << "[Update] 成功更新 " << updated_count << " 行" << std::endl;
    }

    void Executor::executeSelect(const PlanNode &plan)
    {
        if (!catalog_ || !catalog_->HasTable(plan.table_name))
        {
            std::cerr << "[Select] 表不存在" << std::endl;
            return;
        }

        const auto &schema = catalog_->GetTable(plan.table_name);
        if (!storage_engine_)
        {
            std::cerr << "[Select] StorageEngine 未初始化！" << std::endl;
            return;
        }

        size_t num_pages = storage_engine_->GetNumPages();
        int count = 0;

        for (page_id_t pid = 1; pid < static_cast<page_id_t>(num_pages); ++pid)
        {
            Page *p = storage_engine_->GetDataPage(pid);
            if (!p)
                continue;

            auto records = storage_engine_->GetPageRecords(p);

            for (auto &rec : records)
            {
                auto row = Row::Deserialize(reinterpret_cast<const unsigned char *>(rec.first),
                                            rec.second, schema);

                if (!matchesPredicate(row, plan.predicate))
                    continue;

                count++;
                std::cout << "[Row] {";
                for (size_t i = 0; i < plan.columns.size(); ++i)
                {
                    std::string val;
                    for (auto &c : row.columns)
                    {
                        if (c.col_name == plan.columns[i])
                        {
                            val = c.value;
                            break;
                        }
                    }
                    std::cout << plan.columns[i] << ": " << val;
                    if (i < plan.columns.size() - 1)
                        std::cout << ", ";
                }
                std::cout << "}" << std::endl;
            }
        }

        std::cout << "[Select] 返回 " << count << " 行" << std::endl;
    }

} // namespace minidb
