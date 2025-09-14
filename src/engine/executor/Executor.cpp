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
#include "../../storage/page/page_utils.h" // <-- 新增，用于 GetRow / GetSlotCount / HasSpaceFor 等
#include "../operators/row.h"
#include "../../util/config.h"     // PAGE_SIZE, DEFAULT_MAX_PAGES (如果有)
#include "../../util/status.h"     // Status
#include "../../catalog/catalog.h" // Catalog
#include "../../util/table_utils.h" // TableUtils
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

        auto trim = [](const std::string &s) -> std::string
        {
            size_t start = s.find_first_not_of(" \t\n\r");
            size_t end = s.find_last_not_of(" \t\n\r");
            return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
        };

        std::string trimmed = trim(predicate);

        // 支持 col1 = col2 或 col = value
        auto pos_eq = trimmed.find('=');
        if (pos_eq != std::string::npos)
        {
            std::string left = trim(trimmed.substr(0, pos_eq));
            std::string right = trim(trimmed.substr(pos_eq + 1));

            // 判断右侧是列还是常量
            std::string left_val = row.getValue(left);
            std::string right_val = row.getValue(right);

            if (!right_val.empty())
            {
                // col1 = col2
                return left_val == right_val;
            }
            else
            {
                // col = constant
                return left_val == right;
            }
        }

        // 支持 >, <
        auto handleCmp = [&](char op) -> bool
        {
            size_t pos = trimmed.find(op);
            if (pos == std::string::npos)
                return false;
            std::string col = trim(trimmed.substr(0, pos));
            std::string val = trim(trimmed.substr(pos + 1));
            std::string row_val_str = trim(row.getValue(col));
            try
            {
                int row_val = std::stoi(row_val_str);
                int cmp_val = std::stoi(val);
                if (op == '>')
                    return row_val > cmp_val;
                else
                    return row_val < cmp_val;
            }
            catch (...)
            {
                return false;
            }
        };

        if (trimmed.find('>') != std::string::npos)
            return handleCmp('>');
        if (trimmed.find('<') != std::string::npos)
            return handleCmp('<');

        return false;
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

    inline bool parsePredicate(const std::string &pred, std::string &col, std::string &val)
    {
        auto pos = pred.find('=');
        if (pos == std::string::npos)
            return false;

        col = pred.substr(0, pos);
        val = pred.substr(pos + 1);

        // 去掉空格
        auto trim = [](std::string &s)
        {
            size_t start = s.find_first_not_of(" \t");
            size_t end = s.find_last_not_of(" \t");
            if (start == std::string::npos || end == std::string::npos)
            {
                s.clear();
                return;
            }
            s = s.substr(start, end - start + 1);
        };

        trim(col);
        trim(val);

        return !col.empty() && !val.empty();
    }

    // select * 的实现
    std::vector<std::string> Executor::expandWildcardColumns(const PlanNode *node)
    {
        std::vector<std::string> expanded;

        // 如果 JSON 里有 from_tables（多表）
        if (!node->from_tables.empty())
        {
            for (auto &table_name : node->from_tables)
            {
                if (!catalog_->HasTable(table_name))
                    continue;

                auto schema = catalog_->GetTable(table_name);
                for (auto &col : schema.columns)
                {
                    // 多表情况加前缀，避免歧义
                    expanded.push_back(table_name + "." + col.name);
                }
            }
        }
        else
        {
            // 单表情况，取 node->table_name
            if (!catalog_->HasTable(node->table_name))
                return expanded;

            auto schema = catalog_->GetTable(node->table_name);
            for (auto &col : schema.columns)
            {
                expanded.push_back(col.name); // 单表可以不加前缀
            }
        }

        return expanded;
    }

    /**
     * EXECUTOR the given plan node
     * Note: This is a simplified implementation and does not cover all edge cases or optimizations
     */
    std::vector<Row> Executor::execute(PlanNode *node)
    {
        if (!node)
        {
            std::cerr << "[Executor] Null PlanNode" << std::endl;
            return {};
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
            return {};
        }

        case PlanType::Insert:
        {
            logger.log("INSERT INTO " + node->table_name);
            std::cout << "[Executor] 插入到表: " << node->table_name << std::endl;

            if (!storage_engine_)
            {
                std::cerr << "[Executor] StorageEngine 未初始化！" << std::endl;
                return {};
            }
            if (!catalog_ || !catalog_->HasTable(node->table_name))
            {
                std::cerr << "[Executor] Table not found in catalog: " << node->table_name << std::endl;
                return {};
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
                    return {};
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
                        return {};
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
                        return {};
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
                        return {};
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
            catalog_->SaveToStorage(); // 写回页0的Catalog元数据（含首页信息）

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
            catalog_->SaveToStorage();

            return {};
        }
        // SeqScan（修复）
        case PlanType::SeqScan:
        {
            logger.log("SEQSCAN " + node->table_name);
            std::cout << "[Executor] 顺序扫描表: " << node->table_name << std::endl;

            auto rows = SeqScanAll(node->table_name);
            std::cout << "[SeqScan] 扫描到 " << rows.size() << " 行:" << std::endl;

            // 只在调试时显示前几行，避免输出过多
            size_t show_count = std::min(rows.size(), size_t(5));
            for (size_t i = 0; i < show_count; ++i)
            {
                std::cout << "[Row] " << rows[i].toString() << std::endl;
            }
            if (rows.size() > show_count)
            {
                std::cout << "[SeqScan] ... 还有 " << (rows.size() - show_count) << " 行" << std::endl;
            }
            // ===== 添加表格输出 =====
            // TablePrinter::printResults(rows, "SELECT");
            return rows;
        }

        case PlanType::Delete:
        {
            logger.log("DELETE FROM " + node->table_name + " WHERE " + node->predicate);
            std::cout << "[Executor] 删除表: " << node->table_name
                      << " WHERE " << node->predicate << std::endl;

            if (!storage_engine_ || !catalog_)
                return {};

            const auto &schema = catalog_->GetTable(node->table_name);
            size_t deleted = 0;

            // ===== 优先查找索引 =====
            // 这里假设 predicate 是 "col = value" 这种简单形式
            std::string col, value;
            if (parsePredicate(node->predicate, col, value)) // 你需要写个 parsePredicate 简单解析
            {
                auto idx_name = catalog_->FindIndexByColumn(node->table_name, col);
                if (!idx_name.empty())
                {
                    const auto &idx_schema = catalog_->GetIndex(idx_name);
                    if (idx_schema.type == "BPLUS")
                    {
                        BPlusTree bpt(storage_engine_.get()); // ✅ 构造函数只接受 StorageEngine*

                        // 用索引定位要删除的 key
                        int64_t key = std::stoll(value);
                        auto rid_opt = bpt.Search(static_cast<int32_t>(key)); // 返回 optional<RID>

                        if (rid_opt.has_value())
                        {
                            RID rid = rid_opt.value();

                            Page *p = storage_engine_->GetDataPage(rid.page_id);
                            if (p)
                            {
                                auto records = storage_engine_->GetPageRecords(p);
                                std::vector<std::pair<const void *, uint16_t>> new_records;

                                const minidb::TableSchema &schema = catalog_->GetTable(node->table_name);
                                for (auto &rec : records)
                                {
                                    auto row = Row::Deserialize(
                                        reinterpret_cast<const unsigned char *>(rec.first),
                                        rec.second,
                                        schema);
                                    if (!matchesPredicate(row, node->predicate))
                                    {
                                        new_records.push_back(rec);
                                    }
                                    else
                                    {
                                        ++deleted;
                                        // 从 B+ 树索引中删除 key
                                        bpt.Delete(static_cast<int32_t>(key));
                                    }
                                }

                                // 重写数据页
                                p->InitializePage(PageType::DATA_PAGE);
                                for (auto &rec : new_records)
                                {
                                    storage_engine_->AppendRecordToPage(p, rec.first, rec.second);
                                }
                                storage_engine_->PutPage(rid.page_id, true);
                            }
                        }

                        std::cout << "[Delete] 使用索引共删除 " << deleted << " 行" << std::endl;
                        return {}; // ✅ 用索引删除完直接返回
                    }
                }
            }

            // ===== 没有索引，退回全表扫描 =====
            size_t num_pages = storage_engine_->GetNumPages();
            for (page_id_t pid = 1; pid < static_cast<page_id_t>(num_pages); ++pid)
            {
                Page *p = storage_engine_->GetDataPage(pid);
                if (!p)
                    continue;

                auto records = storage_engine_->GetPageRecords(p);
                std::vector<std::pair<const void *, uint16_t>> new_records;

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
                    p->InitializePage(PageType::DATA_PAGE);
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
            return {};
        }

        // ===== Filter =====（修复）
        case PlanType::Filter:
        {
            logger.log("FILTER on " + node->predicate);
            std::cout << "[Executor] 过滤条件: " << node->predicate << std::endl;
            // 从子节点获取数据
            std::vector<Row> input_rows;
            if (!node->children.empty())
            {
                input_rows = execute(node->children[0].get());
            }
            else
            {
                // 没有子节点，直接全表扫描
                if (catalog_ && catalog_->HasTable(node->table_name))
                {
                    input_rows = SeqScanAll(node->table_name);
                }
            }

            // 应用过滤条件
            std::vector<Row> filtered;
            for (const auto &row : input_rows)
            {
                if (matchesPredicate(row, node->predicate))
                {
                    filtered.push_back(row);
                }
            }

            std::cout << "[Filter] 过滤后 " << filtered.size() << " 行:" << std::endl;
            for (auto &row : filtered)
                std::cout << "[Row] " << row.toString() << std::endl;
            // ===== 添加表格输出 =====
            // TablePrinter::printResults(filtered, "SELECT");
            return filtered;
        }

        // ===== Project =====（修复）
        case PlanType::Project:
        {
            logger.log("PROJECT columns");
            std::cout << "[Executor] 投影列: ";

            // 处理 SELECT * 的情况
            std::vector<std::string> projection_columns;
            if (node->columns.empty())
            {
                // SELECT * - 获取表的所有列
                std::cout << "* (所有列)";
                if (catalog_ && catalog_->HasTable(node->table_name))
                {
                    const auto &schema = catalog_->GetTable(node->table_name);
                    for (const auto &col : schema.columns)
                    {
                        projection_columns.push_back(col.name);
                    }
                }
            }
            else
            {
                // 具体列名
                projection_columns = node->columns;
                for (auto &col : projection_columns) // 将node->columns换为projection_columns
                    std::cout << col << " ";
            }
            std::cout << std::endl;
            // 从子节点获取数据
            std::vector<Row> input_rows;
            if (!node->children.empty())
            {
                // 有子节点，执行子节点获取数据
                input_rows = execute(node->children[0].get());
            }
            else
            {
                // 没有子节点，直接全表扫描
                if (catalog_ && catalog_->HasTable(node->table_name))
                {
                    input_rows = SeqScanAll(node->table_name);
                }
            }

            // 执行投影
            std::vector<Row> projected;
            for (const auto &input_row : input_rows)
            {
                Row projected_row;

                if (projection_columns.empty())
                {
                    // 如果投影列仍为空，返回所有列
                    projected_row = input_row;
                }
                else
                {
                    // 按指定列进行投影
                    for (const auto &col_name : projection_columns)
                    {
                        // 在输入行中查找对应列
                        bool found = false;
                        for (const auto &input_col : input_row.columns)
                        {
                            if (input_col.col_name == col_name)
                            {
                                projected_row.columns.push_back(input_col);
                                found = true;
                                break;
                            }
                        }

                        if (!found)
                        {
                            // 如果没找到列，添加空值
                            ColumnValue cv;
                            cv.col_name = col_name;
                            cv.value = "";
                            projected_row.columns.push_back(cv);
                        }
                    }
                }

                projected.push_back(projected_row);
            }

            std::cout << "[Project] 投影后 " << projected.size() << " 行:" << std::endl;
            for (auto &row : projected)
                std::cout << "[Row] " << row.toString() << std::endl;
            // ===== 添加表格输出 =====
            // TablePrinter::printResults(projected, "SELECT");
            return projected;
        }

        case PlanType::Update:
            Update(*node);
            return {};

        case PlanType::GroupBy:
        {
            logger.log("GROUP BY on " + node->table_name);
            std::cout << "[Executor] GroupBy 执行，表: " << node->table_name << std::endl;

            if (!catalog_ || !catalog_->HasTable(node->table_name))
                break;

            // Step1: 先从子节点拿数据
            std::vector<Row> rows;
            if (!node->children.empty())
                rows = execute(node->children[0].get());
            else
                rows = SeqScanAll(node->table_name);

            // Step2: 按 group_keys 分组
            std::map<std::string, std::vector<Row>> groups;
            for (auto &row : rows)
            {
                std::string key;
                for (auto &col : node->group_keys)
                    key += row.getValue(col) + "|";
                groups[key].push_back(row);
            }

            // Step3: 聚合
            std::vector<Row> result_rows;
            for (auto &[gkey, grows] : groups)
            {
                Row out;

                // 分组键
                for (auto &col : node->group_keys)
                {
                    ColumnValue cv;
                    cv.col_name = col;
                    cv.value = grows[0].getValue(col);
                    out.columns.push_back(cv);
                }

                // 聚合函数
                for (auto &agg : node->aggregates)
                {
                    std::string val;

                    if (agg.func == "COUNT")
                        val = std::to_string(grows.size());
                    else if (agg.func == "SUM")
                    {
                        long long sum = 0;
                        for (auto &r : grows)
                            sum += std::stoll(r.getValue(agg.column));
                        val = std::to_string(sum);
                    }
                    else if (agg.func == "AVG")
                    {
                        long long sum = 0;
                        for (auto &r : grows)
                            sum += std::stoll(r.getValue(agg.column));
                        double avg = grows.empty() ? 0.0 : (double)sum / grows.size();
                        val = std::to_string(avg);
                    }
                    else if (agg.func == "MIN")
                    {
                        long long m = LLONG_MAX;
                        for (auto &r : grows)
                            m = std::min(m, std::stoll(r.getValue(agg.column)));
                        val = std::to_string(m);
                    }
                    else if (agg.func == "MAX")
                    {
                        long long m = LLONG_MIN;
                        for (auto &r : grows)
                            m = std::max(m, std::stoll(r.getValue(agg.column)));
                        val = std::to_string(m);
                    }

                    ColumnValue cv;
                    cv.col_name = agg.as_name.empty() ? agg.func + "(" + agg.column + ")" : agg.as_name;
                    cv.value = val;
                    out.columns.push_back(cv);
                }

                result_rows.push_back(out);
            }

            // Step4: 如果有 HAVING 条件，统一过滤
            if (!node->having_predicate.empty())
            {
                std::vector<Row> filtered;
                for (auto &row : result_rows)
                {
                    if (matchesPredicate(row, node->having_predicate))
                        filtered.push_back(row);
                }
                result_rows = std::move(filtered);

            }
            // std::cout << "[GroupBy] 结果: " << result_rows.size() << " 行" << std::endl;
            // for (auto &r : result_rows){
            //     std::cout <<  "[Row] " << r.toString() << std::endl;
            // }
            // ===== 新增：输出结果 =====
            if (!node->having_predicate.empty()) {
                // TablePrinter::printResults(result_rows, "GROUP BY with HAVING");
            } else {
                // TablePrinter::printResults(result_rows, "GROUP BY");
            }
            return result_rows;
        }

        case PlanType::Having:
        {
            logger.log("HAVING " + node->predicate);
            std::cout << "[Executor] Having 执行，条件: " << node->predicate << std::endl;

            if (node->children.empty())
                return {}; // 没有子节点直接返回空

            // 执行子节点（通常是 GroupBy）
            auto rows = execute(node->children[0].get());
            std::vector<Row> result_rows;

            // 过滤每一行，使用你的 matchesPredicate
            for (auto &row : rows)
            {
                if (matchesPredicate(row, node->predicate))
                    result_rows.push_back(row);
            }

            std::cout << "[Having] 过滤后结果: " << result_rows.size() << " 行" << std::endl;
            for (auto &r : result_rows)
                std::cout << r.toString() << std::endl;

            return result_rows;
        }

        case PlanType::Join:
        {
            logger.log("JOIN tables");

            // 自动生成 SeqScan 子节点（如果 children 为空且 from_tables 至少有两个表）
            if (node->children.empty() && node->from_tables.size() >= 2)
            {
                for (auto &tbl : node->from_tables)
                {
                    auto scan = std::make_unique<PlanNode>();
                    scan->type = PlanType::SeqScan;
                    scan->table_name = tbl;
                    node->children.push_back(std::move(scan));
                }
            }

            if (node->children.size() < 2)
            {
                std::cerr << "[Join] 需要至少两个子节点" << std::endl;
                return {};
            }

            // 扫描第一个子节点
            std::vector<Row> joined_rows = execute(node->children[0].get());
            // 给列加表名前缀
            for (auto &row : joined_rows)
                for (auto &col : row.columns)
                    if (col.col_name.find('.') == std::string::npos)
                        col.col_name = node->from_tables[0] + "." + col.col_name;

            // 对后续子节点依次 JOIN
            for (size_t i = 1; i < node->children.size(); ++i)
            {
                std::vector<Row> right_rows = execute(node->children[i].get());
                // 给右表列加前缀
                for (auto &row : right_rows)
                    for (auto &col : row.columns)
                        if (col.col_name.find('.') == std::string::npos)
                            col.col_name = node->from_tables[i] + "." + col.col_name;

                std::vector<Row> tmp;

                std::string join_predicate = node->predicate;
                auto trim = [](const std::string &s) -> std::string
                {
                    size_t start = s.find_first_not_of(" \t\n\r");
                    size_t end = s.find_last_not_of(" \t\n\r");
                    return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
                };
                auto pos_eq = join_predicate.find('=');
                if (pos_eq == std::string::npos)
                {
                    std::cerr << "[Join] 仅支持 col=col 条件" << std::endl;
                    return {};
                }

                std::string left_col = trim(join_predicate.substr(0, pos_eq));
                std::string right_col = trim(join_predicate.substr(pos_eq + 1));

                for (auto &lrow : joined_rows)
                {
                    for (auto &rrow : right_rows)
                    {
                        std::string lval = lrow.getValue(left_col);
                        std::string rval = rrow.getValue(right_col);

                        if (lval == rval)
                        {
                            Row combined = lrow;
                            combined.columns.insert(combined.columns.end(), rrow.columns.begin(), rrow.columns.end());
                            tmp.push_back(std::move(combined));
                        }
                    }
                }

                joined_rows.swap(tmp);
            }

            std::cout << "[Join] 连接后 " << joined_rows.size() << " 行:" << std::endl;
            for (auto &row : joined_rows)
                std::cout << "[Row] " << row.toString() << std::endl;

            return joined_rows;
        }

        case PlanType::OrderBy:
        {
            logger.log("ORDER BY");

            if (node->children.empty())
            {
                std::cerr << "[OrderBy] 缺少子节点" << std::endl;
                return {};
            }

            std::vector<Row> rows = execute(node->children[0].get());
            if (rows.empty() || node->order_by_cols.empty())
                return rows;

            TableSchema schema = catalog_->GetTable(node->children[0]->table_name);

            // 单列索引优化
            bool use_index = false;
            std::string index_name;
            int order_col_idx = -1;

            if (node->order_by_cols.size() == 1)
            {
                index_name = catalog_->FindIndexByColumn(schema.table_name, node->order_by_cols[0]);
                order_col_idx = schema.getColumnIndex(node->order_by_cols[0]);
                use_index = (!index_name.empty() && order_col_idx != -1);
            }

            if (use_index)
            {
                logger.log("[OrderBy] 使用 B+ 树索引");

                BPlusTree tree(storage_engine_.get());
                // 索引 root 已经在 IndexSchema 中
                IndexSchema idx = catalog_->GetIndex(index_name);
                tree.SetRoot(idx.root_page_id);

                // 获取按索引顺序的 RID
                std::vector<RID> rids = tree.Range(INT32_MIN, INT32_MAX);

                rows.clear();
                for (auto &rid : rids)
                {
                    Page *p = storage_engine_->GetDataPage(rid.page_id);
                    if (!p)
                        continue;

                    auto records = storage_engine_->GetPageRecords(p);

                    // 用 slot 直接访问记录
                    if (rid.slot < records.size())
                    {
                        const auto &rec = records[rid.slot];
                        Row row = Row::Deserialize(reinterpret_cast<const unsigned char *>(rec.first), rec.second, schema);
                        rows.push_back(row);
                    }
                }

                if (node->order_by_desc)
                    std::reverse(rows.begin(), rows.end());
            }
            else
            {
                // 普通内存排序
                std::vector<int> order_col_idxs;
                std::vector<std::string> order_col_types;
                for (auto &col : node->order_by_cols)
                {
                    int idx = schema.getColumnIndex(col);
                    if (idx == -1)
                    {
                        std::cerr << "[OrderBy] 列不存在: " << col << std::endl;
                        return {};
                    }
                    order_col_idxs.push_back(idx);
                    order_col_types.push_back(schema.columns[idx].type);
                }

                std::sort(rows.begin(), rows.end(), [&](const Row &a, const Row &b)
                          {
                              for (size_t i = 0; i < order_col_idxs.size(); ++i)
                              {
                                  const std::string &aval_str = a.columns[order_col_idxs[i]].value;
                                  const std::string &bval_str = b.columns[order_col_idxs[i]].value;
                                  const std::string &type = order_col_types[i];

                                  bool cmp_result = false;

                                  if (type == "INT")
                                  {
                                      int aval = aval_str.empty() ? 0 : std::stoi(aval_str);
                                      int bval = bval_str.empty() ? 0 : std::stoi(bval_str);
                                      cmp_result = node->order_by_desc ? aval > bval : aval < bval;
                                      if (aval != bval)
                                          return cmp_result;
                                  }
                                  else if (type == "DOUBLE")
                                  {
                                      double aval = aval_str.empty() ? 0.0 : std::stod(aval_str);
                                      double bval = bval_str.empty() ? 0.0 : std::stod(bval_str);
                                      cmp_result = node->order_by_desc ? aval > bval : aval < bval;
                                      if (aval != bval)
                                          return cmp_result;
                                  }
                                  else // 字符串类型
                                  {
                                      cmp_result = node->order_by_desc ? aval_str > bval_str : aval_str < bval_str;
                                      if (aval_str != bval_str)
                                          return cmp_result;
                                  }
                              }
                              return false; // 所有排序列相等
                          });
            }

            std::cout << "[OrderBy] 排序后 " << rows.size() << " 行" << std::endl;
            return rows;
        }

        default:
            std::cerr << "[Executor] 未知 PlanNode 类型" << std::endl;
            return {};
        }
        return {};
    }

    // Helper: 从 RID 定位并反序列化一行（返回 optional）
    static std::optional<Row> FetchRowByRID(StorageEngine *storage_engine,
                                            const RID &rid,
                                            const minidb::TableSchema &schema)
    {
        if (!storage_engine)
            return std::nullopt;
        Page *page = storage_engine->GetDataPage(rid.page_id);
        if (!page)
            return std::nullopt;

        // 检查 slot 合法性
        uint16_t slot_count = page->GetSlotCount();
        if (rid.slot >= slot_count)
        {
            // slot 不合法，释放页并返回空
            storage_engine->PutPage(page->GetPageId(), false);
            return std::nullopt;
        }

        // 通过 page_utils 提取记录指针与长度
        uint16_t rec_len = 0;
        const unsigned char *rec_ptr = minidb::GetRow(page, rid.slot, &rec_len);
        if (!rec_ptr || rec_len == 0)
        {
            storage_engine->PutPage(page->GetPageId(), false);
            return std::nullopt;
        }

        Row row = Row::Deserialize(rec_ptr, rec_len, schema);
        storage_engine->PutPage(page->GetPageId(), false);
        return row;
    }

    // 单页扫描（保持原样）
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

    // 遍历整张表 —— 优先尝试使用单列 B+ 树索引作全表扫描（Range(-INF,+INF)）
    // 否则回退到页链全表扫描（原实现）
    std::vector<Row> Executor::SeqScanAll(const std::string &table_name)
    {
        std::vector<Row> all_rows;
        if (!storage_engine_ || !catalog_)
            return all_rows;

        const auto &schema = catalog_->GetTable(table_name);

        // 1) 尝试找到可用的单列 B+ 索引（优先第一个满足条件的）
        std::vector<IndexSchema> idxs = catalog_->GetTableIndexes(table_name);
        IndexSchema usable_idx;
        bool found_idx = false;
        for (const auto &idx : idxs)
        {
            if (idx.type == "BPLUS" && idx.cols.size() == 1)
            {
                // 这里只简单认为单列索引可以直接用于全表遍历（叶链遍历）
                usable_idx = idx;
                found_idx = true;
                break;
            }
        }

        if (found_idx && usable_idx.root_page_id != INVALID_PAGE_ID)
        {
            // 使用 B+ 树做全表扫描（通过 range 从最小键到最大键）
            BPlusTree bpt(storage_engine_.get());
            bpt.SetRoot(usable_idx.root_page_id);

            // Range 用 int32 的边界进行整表遍历（你的 BPlusTree.Range 返回 RID 列表）
            auto rids = bpt.Range(INT32_MIN, INT32_MAX);
            all_rows.reserve(rids.size());

            for (const RID &rid : rids)
            {
                auto maybe_row = FetchRowByRID(storage_engine_.get(), rid, schema);
                if (maybe_row.has_value())
                    all_rows.push_back(std::move(maybe_row.value()));
                // 否则跳过（可能索引指向已删除或损坏的行）
            }

            return all_rows;
        }

        // 回退：原有的页链式全表扫描
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

        // ===== 尝试使用索引 =====
        std::string col, value;
        bool use_index = false;
        BPlusTree *bpt_ptr = nullptr;
        IndexSchema idx_schema;

        if (parsePredicate(plan.predicate, col, value))
        {
            auto idx_name = catalog_->FindIndexByColumn(plan.table_name, col);
            if (!idx_name.empty())
            {
                idx_schema = catalog_->GetIndex(idx_name);
                int col_idx = schema.getColumnIndex(col);
                bool is_int_col = (col_idx >= 0 && col_idx < (int)schema.columns.size() &&
                                   schema.columns[col_idx].type == "INT");
                if (is_int_col && idx_schema.type == "BPLUS")
                {
                    use_index = true;
                    bpt_ptr = new BPlusTree(storage_engine_.get());
                    bpt_ptr->SetRoot(idx_schema.root_page_id);
                }
            }
        }

        if (use_index && bpt_ptr)
        {
            // 通过索引找到对应 RID
            int32_t key = static_cast<int32_t>(std::stoll(value));
            auto rid_opt = bpt_ptr->Search(key);
            if (rid_opt.has_value())
            {
                RID rid = rid_opt.value();
                Page *p = storage_engine_->GetDataPage(rid.page_id);
                if (p)
                {
                    auto records = storage_engine_->GetPageRecords(p);
                    std::vector<std::vector<char>> new_records;
                    bool page_modified = false;

                    for (auto &rec : records)
                    {
                        auto row = Row::Deserialize(reinterpret_cast<const unsigned char *>(rec.first),
                                                    rec.second, schema);

                        if (matchesPredicate(row, plan.predicate))
                        {
                            int32_t old_key = key;
                            int32_t new_key = old_key;
                            bool update_index = false;

                            for (const auto &kv : plan.set_values)
                            {
                                row.setValue(kv.first, kv.second);
                                if (kv.first == col)
                                {
                                    update_index = true;
                                    new_key = static_cast<int32_t>(std::stoll(row.getValue(col)));
                                }
                            }

                            std::vector<char> buf;
                            row.Serialize(buf, schema);
                            new_records.push_back(std::move(buf));

                            page_modified = true;
                            ++updated_count;

                            // 更新索引
                            if (update_index)
                            {
                                bpt_ptr->Delete(old_key);
                                bpt_ptr->Insert(new_key, rid);
                            }
                        }
                        else
                        {
                            // 保留原始记录
                            new_records.emplace_back(
                                reinterpret_cast<const char *>(rec.first),
                                reinterpret_cast<const char *>(rec.first) + rec.second);
                        }
                    }

                    // 写回页
                    if (page_modified)
                    {
                        p->InitializePage(PageType::DATA_PAGE);
                        for (auto &rec : new_records)
                            storage_engine_->AppendRecordToPage(p, rec.data(), rec.size());
                        storage_engine_->PutPage(rid.page_id, true);
                    }
                    else
                    {
                        storage_engine_->PutPage(rid.page_id, false);
                    }
                }
            }
            else
            {
                // 如果索引没有找到，fallback 全表扫描
                use_index = false;
            }
        }

        if (!use_index)
        {
            // ===== 全表扫描更新 =====
            for (page_id_t pid = 1; pid < static_cast<page_id_t>(num_pages); ++pid)
            {
                Page *p = storage_engine_->GetDataPage(pid);
                if (!p)
                    continue;

                auto records = storage_engine_->GetPageRecords(p);
                std::vector<std::vector<char>> new_records;
                bool page_modified = false;

                for (auto &rec : records)
                {
                    auto row = Row::Deserialize(reinterpret_cast<const unsigned char *>(rec.first),
                                                rec.second, schema);

                    if (matchesPredicate(row, plan.predicate))
                    {
                        for (const auto &kv : plan.set_values)
                            row.setValue(kv.first, kv.second);

                        std::vector<char> buf;
                        row.Serialize(buf, schema);
                        new_records.push_back(std::move(buf));

                        page_modified = true;
                        ++updated_count;
                    }
                    else
                    {
                        new_records.emplace_back(
                            reinterpret_cast<const char *>(rec.first),
                            reinterpret_cast<const char *>(rec.first) + rec.second);
                    }
                }

                if (page_modified)
                {
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
        }

        delete bpt_ptr; // 释放 BPlusTree
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

        std::vector<Row> results;
        bool use_index = false;

        // 尝试使用索引（假设 predicate 为 "col = value" 形式）
        std::string col, value;
        if (parsePredicate(plan.predicate, col, value))
        {
            auto idx_name = catalog_->FindIndexByColumn(plan.table_name, col);
            if (!idx_name.empty())
            {
                const auto &idx_schema = catalog_->GetIndex(idx_name);
                if (idx_schema.type == "BPLUS")
                {
                    use_index = true;
                    BPlusTree bpt(storage_engine_.get());
                    bpt.SetRoot(idx_schema.root_page_id); // 设置已有索引根页

                    int64_t key = std::stoll(value);
                    auto rid_opt = bpt.Search(static_cast<int32_t>(key));

                    if (rid_opt.has_value())
                    {
                        RID rid = rid_opt.value();
                        Page *p = storage_engine_->GetDataPage(rid.page_id);
                        if (p)
                        {
                            auto records = storage_engine_->GetPageRecords(p);
                            for (auto &rec : records)
                            {
                                auto row = Row::Deserialize(reinterpret_cast<const unsigned char *>(rec.first),
                                                            rec.second, schema);
                                if (matchesPredicate(row, plan.predicate))
                                    results.push_back(row);
                            }
                        }
                    }
                }
            }
        }

        // fallback 全表扫描
        if (!use_index)
        {
            size_t num_pages = storage_engine_->GetNumPages();
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
                    if (matchesPredicate(row, plan.predicate))
                        results.push_back(row);
                }
            }
        }

        // 输出结果
        int count = 0;
        for (auto &row : results)
        {
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

        std::cout << "[Select] 返回 " << count << " 行" << std::endl;
    }

} // namespace minidb