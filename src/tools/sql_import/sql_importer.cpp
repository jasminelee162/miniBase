#include "sql_importer.h"
#include "../../engine/executor/executor.h"
#include "../../catalog/catalog.h"
#include "../../engine/operators/plan_node.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

namespace minidb
{

    SQLImporter::SQLImporter(Executor *exec, Catalog *catalog)
        : exec_(exec), catalog_(catalog) {}

    bool SQLImporter::ExecuteSQLFile(const std::string &filename)
    {
        std::ifstream infile(filename);
        if (!infile.is_open())
        {
            std::cerr << "[SQLImporter] 无法打开文件: " << filename << std::endl;
            return false;
        }

        std::string line;
        std::string sql; // 累积 SQL 语句
        while (std::getline(infile, line))
        {
            // 去掉行首尾空格
            line.erase(0, line.find_first_not_of(" \t\n\r"));
            line.erase(line.find_last_not_of(" \t\n\r") + 1);

            if (line.empty() || line[0] == '-')
                continue; // 跳过空行或注释

            sql += line;
            if (line.find(';') != std::string::npos)
            {
                // 执行完整 SQL
                if (!ExecuteSQL(sql))
                {
                    std::cerr << "[SQLImporter] 执行失败: " << sql << std::endl;
                }
                sql.clear();
            }
        }

        return true;
    }

    bool SQLImporter::ImportSQLFile(const std::string &filename)
    {
        std::ifstream infile(filename);
        if (!infile.is_open())
        {
            std::cerr << "[SQLImporter] 无法打开文件: " << filename << std::endl;
            return false;
        }

        std::string line, sql;
        while (std::getline(infile, line))
        {
            sql += line;
            if (line.find(';') != std::string::npos)
            {
                if (!ExecuteSQL(sql))
                {
                    std::cerr << "[SQLImporter] 执行失败: " << sql << std::endl;
                }
                sql.clear();
            }
        }
        return true;
    }

    bool SQLImporter::ExecuteSQL(const std::string &sql)
    {
        std::string stmt = sql;
        if (!stmt.empty() && stmt.back() == ';')
            stmt.pop_back();

        if (stmt.find("CREATE TABLE") == 0)
        {
            return HandleCreate(stmt);
        }
        else if (stmt.find("INSERT INTO") == 0)
        {
            return HandleInsert(stmt);
        }
        else
        {
            std::cerr << "[SQLImporter] 未支持的语句: " << stmt << std::endl;
            return false;
        }
    }

    // ======= CREATE TABLE =======
    bool SQLImporter::HandleCreate(const std::string &sql)
    {
        // 简单解析 CREATE TABLE students (id INT, name VARCHAR(50))
        size_t p1 = sql.find("(");
        size_t p2 = sql.find(")");
        if (p1 == std::string::npos || p2 == std::string::npos || p2 <= p1)
            return false;

        std::string header = sql.substr(0, p1);
        std::string cols = sql.substr(p1 + 1, p2 - p1 - 1);

        std::istringstream hs(header);
        std::string tmp, tableName;
        hs >> tmp >> tmp >> tableName; // CREATE TABLE <tableName>

        PlanNode ct;
        ct.type = PlanType::CreateTable;
        ct.table_name = tableName;

        std::istringstream cs(cols);
        std::string colDef;
        while (std::getline(cs, colDef, ','))
        {
            std::istringstream cd(colDef);
            std::string cname, ctype;
            cd >> cname >> ctype;
            int len = -1;
            if (ctype.find("VARCHAR") != std::string::npos)
            {
                size_t l = ctype.find("("), r = ctype.find(")");
                len = std::stoi(ctype.substr(l + 1, r - l - 1));
                ctype = "VARCHAR";
            }
            ct.table_columns.push_back({cname, ctype, len});
        }

        // 执行创建
        exec_->execute(&ct);

        // 确保 Catalog 页写入，并更新 MetaPage.catalog_root
        catalog_->SaveToStorage();

        return true;
    }

    // ======= INSERT INTO =======
    bool SQLImporter::HandleInsert(const std::string &sql)
    {
        // INSERT INTO students VALUES('1','Alice','20')
        size_t p1 = sql.find("INTO");
        size_t p2 = sql.find("VALUES");
        if (p1 == std::string::npos || p2 == std::string::npos)
            return false;

        std::string tableName = sql.substr(p1 + 5, p2 - (p1 + 5));
        tableName.erase(tableName.find_last_not_of(" \t") + 1);

        std::string values = sql.substr(p2 + 6);
        if (values.front() == '(')
            values.erase(values.begin());
        if (values.back() == ')')
            values.pop_back();

        std::istringstream vs(values);
        std::string val;
        std::vector<std::string> row;
        while (std::getline(vs, val, ','))
        {
            val.erase(std::remove(val.begin(), val.end(), '\''), val.end());
            val.erase(0, val.find_first_not_of(" \t"));
            val.erase(val.find_last_not_of(" \t") + 1);
            row.push_back(val);
        }

        // 构建 PlanNode
        PlanNode ins;
        ins.type = PlanType::Insert;
        ins.table_name = tableName;

        // 读取表结构
        TableSchema schema = catalog_->GetTable(tableName);
        ins.columns.clear();
        for (auto &col : schema.columns)
        {
            ins.columns.push_back(col.name);
        }
        ins.values.push_back(row);

        // ===== 关键：先分配数据页（如果没有） =====
        page_id_t first_page_id = schema.first_page_id;
        Page *cur_page = nullptr;
        if (first_page_id == INVALID_PAGE_ID)
        {
            page_id_t new_pid = INVALID_PAGE_ID;
            cur_page = exec_->GetStorageEngine()->CreateDataPage(&new_pid);
            if (!cur_page)
            {
                std::cerr << "[SQLImporter] 无法创建数据页" << std::endl;
                return false;
            }
            schema.first_page_id = new_pid;
            catalog_->CreateTable(schema.table_name, schema.columns); // 更新 Catalog
            catalog_->SaveToStorage();
        }

        // 执行插入
        exec_->execute(&ins);

        return true;
    }

} // namespace minidb
