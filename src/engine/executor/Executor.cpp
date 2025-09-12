// src/engine/executor/executor.cpp
#include "executor.h"

#include <iostream>
#include <cstring>
#include <algorithm>
#include <sstream>
#include <cctype>
#include <functional>

#include "../../util/logger.h"
#include "../../storage/storage_engine.h" // 使用 StorageEngine
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

    // 从 Page 中解析出 Row
    Row parseRowFromPage(Page *p, const std::vector<std::string> &columns)
    {

        Row row;
        if (!p)
            return row;

        char *data = p->GetData();
        size_t offset = 0;

        if (offset + sizeof(int32_t) > PAGE_SIZE)
            return row;

        int32_t col_count = 0;
        std::memcpy(&col_count, data + offset, sizeof(int32_t));
        offset += sizeof(int32_t);

        if (col_count <= 0 || offset + static_cast<size_t>(col_count) * 64 > PAGE_SIZE)
            return row;
        // 打印传入的 columns（便于排查空的情况）
        std::cout << "[DDDDDEBUG] parseRowFromPage col_count=" << col_count << " schema_cols=" << columns.size() << std::endl;
        for (size_t i = 0; i < columns.size(); ++i)
            std::cout << "[DDDDDEBUG] schema[" << i << "]=" << columns[i] << std::endl;

        const size_t FIELD_SIZE = 64;
        size_t tmp_off = offset;
        for (int i = 0; i < col_count; ++i)
        {
            std::string val(data + tmp_off, FIELD_SIZE);
            val = trim_field(val.data(), val.size());

            ColumnValue col;
            if (i < (int)columns.size())
                col.col_name = columns[i]; // 用 schema 的列名
            else
                col.col_name = "col" + std::to_string(i);

            col.value = val;
            row.columns.push_back(col);
            tmp_off += FIELD_SIZE;
        }
        return row;
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

            // 取表的 schema（这里要保证 catalog_ 已经有表的定义）
            TableSchema schema = catalog_->GetTable(node->table_name);

            for (size_t row_idx = 0; row_idx < node->values.size(); ++row_idx)
            {
                Row row;
                for (size_t i = 0; i < node->columns.size(); ++i)
                {
                    ColumnValue col;
                    col.col_name = node->columns[i]; // ✅ Insert.columns 是 string
                    col.value = node->values[row_idx][i];
                    row.columns.push_back(col);
                }

                std::cout << "[DEBUG] Insert row: " << row.toString() << std::endl;

                page_id_t pid;
                Page *p = storage_engine_->CreatePage(&pid);
                if (!p)
                {
                    std::cerr << "[Executor] CreatePage 返回 nullptr" << std::endl;
                    break;
                }

                char *data = p->GetData();
                std::memset(data, 0, PAGE_SIZE);

                size_t offset = 0;
                int32_t col_count = static_cast<int32_t>(row.columns.size());
                std::memcpy(data + offset, &col_count, sizeof(int32_t));
                offset += sizeof(int32_t);

                // 遍历 schema 来存储每一列
                for (size_t i = 0; i < schema.columns.size(); i++)
                {
                    const auto &col_schema = schema.columns[i];
                    std::string val = (i < row.columns.size()) ? row.columns[i].value : "";

                    if (col_schema.type == "INT")
                    {
                        int32_t num = std::stoi(val);
                        std::memcpy(data + offset, &num, sizeof(int32_t));
                        offset += sizeof(int32_t);
                    }
                    else if (col_schema.type == "DOUBLE")
                    {
                        double num = std::stod(val);
                        std::memcpy(data + offset, &num, sizeof(double));
                        offset += sizeof(double);
                    }
                    else if (col_schema.type == "VARCHAR" || col_schema.type == "CHAR")
                    {
                        size_t field_size = (col_schema.length > 0) ? col_schema.length : 64;
                        if (val.size() > field_size)
                            val = val.substr(0, field_size);
                        std::memset(data + offset, 0, field_size);
                        std::memcpy(data + offset, val.data(), val.size());
                        offset += field_size;
                    }
                    else
                    {
                        // fallback: 未知类型，当成字符串存 64 字节
                        size_t field_size = 64;
                        if (val.size() > field_size)
                            val = val.substr(0, field_size);
                        std::memset(data + offset, 0, field_size);
                        std::memcpy(data + offset, val.data(), val.size());
                        offset += field_size;
                    }
                }

                storage_engine_->PutPage(pid, true);
                std::cout << "[Executor] 写页成功，page_id=" << pid
                          << " 行大小=" << offset << " 字节" << std::endl;
            }
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

            for (page_id_t pid = 0; pid < static_cast<page_id_t>(num_pages); ++pid)
            {
                Page *p = storage_engine_->GetPage(pid);
                if (!p)
                    continue;

                std::vector<std::string> columns;
                if (catalog_ && catalog_->HasTable(node->table_name))
                    columns = catalog_->GetTableColumns(node->table_name);
                else if (!node->columns.empty())
                    columns = node->columns;
                else
                    //columns = storage_engine_->GetTableColumns(node->table_name);
                    columns = table_schema;

                Row row = parseRowFromPage(p, columns);
                // auto columns = storage_engine_->GetTableColumns(node->table_name);
                // Row row = parseRowFromPage(p, columns);

                if (matchesPredicate(row, node->predicate))
                {
                    std::memset(p->GetData(), 0, PAGE_SIZE);
                    storage_engine_->PutPage(pid, true);
                    ++deleted;
                    std::cout << "[Delete] 删除 page " << pid << " 成功" << std::endl;
                }
                else
                {
                    storage_engine_->PutPage(pid, false);
                }
            }

            std::cout << "[Delete] 共删除 " << deleted << " 页(行)" << std::endl;
            break;
        }

        case PlanType::Filter:
        {
            logger.log("FILTER on " + node->predicate);
            std::cout << "[Executor] 过滤条件: " << node->predicate << std::endl;

            auto rows = SeqScanAll(node->table_name);
            std::vector<Row> filtered;
            for (auto &row : rows)
            {
                if (matchesPredicate(row, node->predicate))
                    filtered.push_back(row);
            }

            std::cout << "[Filter] 过滤后 " << filtered.size() << " 行:" << std::endl;
            for (auto &row : filtered)
                std::cout << "[Row] " << row.toString() << std::endl;
            break;
        }

        case PlanType::Project:
        {
            logger.log("PROJECT columns");
            std::cout << "[Executor] 投影列: ";
            for (auto &col : node->columns)
                std::cout << col << " "; // ✅ 只打印列名
            std::cout << std::endl;

            auto rows = SeqScanAll(node->table_name);
            std::vector<Row> projected;
            for (auto &row : rows)
            {
                Row r;
                for (auto &col_def : node->columns) // ✅ 用 col_def.name
                {
                    for (auto &col : row.columns)
                    {
                        if (col.col_name == col_def)
                        {
                            r.columns.push_back(col);
                            break;
                        }
                    }
                }
                projected.push_back(r);
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

    // ========== SeqScanAll: 扫描所有已分配页 ==========
    std::vector<Row> Executor::SeqScanAll(const std::string &table_name)
    {
        std::vector<Row> all_rows;
        if (!storage_engine_)
            return all_rows;

        std::vector<std::string> columns;
        if (catalog_ && catalog_->HasTable(table_name))
        {
            std::cout << "[Executor] SeqScanAll: using Catalog for schema" << std::endl;
            columns = catalog_->GetTableColumns(table_name);
        }
        else
        {
            // std::cout << "[Executor] SeqScanAll: using StorageEngine for schema" << std::endl;
            // columns = storage_engine_->GetTableColumns(table_name);
            std::cout << "[Executor] SeqScanAll: using fallback schema" << std::endl;
            // Prefer plan-provided columns; otherwise fallback to default demo schema
            columns = table_schema;
        }

        std::cout << "[Executor] SeqScanAll: got " << columns.size() << " schema cols:";
        for (auto &c : columns)
            std::cout << " '" << c << "'";
        std::cout << std::endl;

        // auto columns = storage_engine_->GetTableColumns(table_name); // 获取表 schema
        size_t num_pages = storage_engine_->GetNumPages();
        std::cout << "[...DEBUG] SeqScanAll scanning " << num_pages << " pages" << std::endl;
        for (page_id_t pid = 0; pid < static_cast<page_id_t>(num_pages); ++pid)
        {
            auto rows = SeqScan(pid, columns);
            if (!rows.empty())
                all_rows.insert(all_rows.end(), rows.begin(), rows.end());
        }
        return all_rows;
    }

    // ========== SeqScan: 单页读取 ==========
    std::vector<Row> Executor::SeqScan(page_id_t page_id, const std::vector<std::string> &columns)
    {
        std::vector<Row> result;
        if (!storage_engine_ || page_id == INVALID_PAGE_ID)
            return result;

        std::cout << "[???DEBUG] SeqScan page_id=" << page_id << std::endl;

        Page *p = storage_engine_->GetPage(page_id);
        if (!p)
            return result;

        Row row = parseRowFromPage(p, columns);
        if (!row.columns.empty())
            result.push_back(row);

        storage_engine_->PutPage(page_id, false);
        return result;
    }

    // ========== Filter 算子 ==========
    std::vector<Row> Executor::Filter(const std::vector<Row> &rows, const std::string &predicate)
    {
        std::vector<Row> result;
        if (predicate.empty())
            return rows;

        for (const auto &row : rows)
        {
            for (const auto &col : row.columns)
            {
                if (col.value.find(predicate) != std::string::npos)
                {
                    result.push_back(row);
                    break;
                }
            }
        }
        return result;
    }

    // ========== Project 算子 ==========
    std::vector<Row> Executor::Project(const std::vector<Row> &rows, const std::vector<std::string> &cols)
    {
        std::vector<Row> result;
        if (cols.empty())
            return rows;

        for (const auto &row : rows)
        {
            Row r;
            for (const auto &col_name : cols)
            {
                for (const auto &col : row.columns)
                {
                    if (col.col_name == col_name)
                    {
                        r.columns.push_back(col);
                        break;
                    }
                }
            }
            result.push_back(r);
        }
        return result;
    }

    void Executor::Update(const PlanNode &plan)
    {
        std::cout << "[Executor] 更新表: " << plan.table_name << " WHERE " << plan.predicate << std::endl;

        if (!catalog_ || !catalog_->HasTable(plan.table_name))
        {
            std::cerr << "[Update] 表不存在或 Catalog 未初始化" << std::endl;
            return;
        }
        TableSchema schema = catalog_->GetTable(plan.table_name);

        if (!storage_engine_)
        {
            std::cerr << "[Update] StorageEngine 未初始化！" << std::endl;
            return;
        }

        size_t num_pages = storage_engine_->GetNumPages(); // ✅
        // size_t num_pages = disk_manager_->GetNumPages();
        int updated_count = 0;
        const size_t FIELD_SIZE = 64;

        for (page_id_t pid = 0; pid < static_cast<page_id_t>(num_pages); ++pid)
        {
            Page *p = storage_engine_->GetPage(pid);
            if (!p)
                continue;

            char *data = p->GetData();
            size_t offset = 0;
            if (offset + sizeof(int32_t) > PAGE_SIZE)
            {
                storage_engine_->PutPage(pid, false);
                continue;
            }

            int32_t col_count = 0;
            std::memcpy(&col_count, data + offset, sizeof(int32_t));
            offset += sizeof(int32_t);
            if (col_count <= 0)
            {
                storage_engine_->PutPage(pid, false);
                continue;
            }

            if (offset + static_cast<size_t>(col_count) * FIELD_SIZE > PAGE_SIZE)
            {
                storage_engine_->PutPage(pid, false);
                continue;
            }

            TableSchema schema = catalog_->GetTable(plan.table_name);
            // 构造 Row（按一页一行）
            Row row;
            size_t tmp_off = offset;
            for (int i = 0; i < col_count; ++i)
            {
                std::string val = trim_field(data + tmp_off, FIELD_SIZE);
                ColumnValue col;
                if (i < static_cast<int>(schema.columns.size()))
                    col.col_name = schema.columns[i].name;
                else
                    col.col_name = "col" + std::to_string(i); // 备用
                col.value = val;
                row.columns.push_back(col);
                tmp_off += FIELD_SIZE;
            }

            bool modified = false;
            if (matchesPredicate(row, plan.predicate))
            {
                // 遍历 Update 的 set_values（列名 -> 新值）
                for (const auto &kv : plan.set_values)
                {
                    row.setValue(kv.first, kv.second); // 更新字段
                    modified = true;
                }
            }

            if (modified)
            {
                // 写回字段区
                tmp_off = offset;
                for (auto &col : row.columns)
                {
                    std::string val = col.value;
                    if (val.size() > FIELD_SIZE)
                        val = val.substr(0, FIELD_SIZE);
                    std::memset(data + tmp_off, 0, FIELD_SIZE);
                    std::memcpy(data + tmp_off, val.data(), val.size());
                    tmp_off += FIELD_SIZE;
                }
                storage_engine_->PutPage(pid, true);
                ++updated_count;
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

        TableSchema schema = catalog_->GetTable(plan.table_name);

        auto rows = SeqScanAll(plan.table_name);
        int count = 0;

        for (auto &row : rows)
        {
            if (!matchesPredicate(row, plan.predicate))
                continue;

            count++;
            std::cout << "[Row] {";
            for (size_t i = 0; i < plan.columns.size(); i++)
            {
                std::string val;
                for (auto &c : row.columns)
                    if (c.col_name == plan.columns[i]) // ✅ 用 name
                    {
                        val = c.value;
                        break;
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
