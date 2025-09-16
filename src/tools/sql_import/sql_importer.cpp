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
            // 去掉行首尾空格
            line.erase(0, line.find_first_not_of(" \t\n\r"));
            line.erase(line.find_last_not_of(" \t\n\r") + 1);
            
            // 跳过空行和注释
            if (line.empty() || line[0] == '-')
                continue;
                
            sql += line + " ";
            if (line.find(';') != std::string::npos)
            {
                std::cout << "[SQLImporter] 执行SQL: " << sql << std::endl;
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
        std::cout << "[SQLImporter] 开始处理CREATE TABLE语句: " << sql << std::endl;
        
        // 简单解析 CREATE TABLE students (id INT, name VARCHAR(50))
        size_t p1 = sql.find("(");
        size_t p2 = sql.find(")");
        if (p1 == std::string::npos || p2 == std::string::npos || p2 <= p1) {
            std::cerr << "[SQLImporter] 无法找到括号" << std::endl;
            return false;
        }

        std::string header = sql.substr(0, p1);
        std::string cols = sql.substr(p1 + 1, p2 - p1 - 1);
        
        std::cout << "[SQLImporter] 表头: " << header << std::endl;
        std::cout << "[SQLImporter] 列定义: " << cols << std::endl;

        std::istringstream hs(header);
        std::string tmp, tableName;
        hs >> tmp >> tmp >> tableName; // CREATE TABLE <tableName>

        PlanNode ct;
        ct.type = PlanType::CreateTable;
        ct.table_name = tableName;

        // 更智能的列解析，处理VARCHAR(100)中的逗号
        std::vector<std::string> colDefs;
        std::string current = "";
        int paren_count = 0;
        
        for (char c : cols) {
            if (c == '(') {
                paren_count++;
                current += c;
            } else if (c == ')') {
                paren_count--;
                current += c;
            } else if (c == ',' && paren_count == 0) {
                colDefs.push_back(current);
                current = "";
            } else {
                current += c;
            }
        }
        if (!current.empty()) {
            colDefs.push_back(current);
        }
        
        std::cout << "[SQLImporter] 解析出 " << colDefs.size() << " 个列定义" << std::endl;
        
        for (size_t i = 0; i < colDefs.size(); i++) {
            const auto& colDef = colDefs[i];
            std::cout << "[SQLImporter] 列定义 " << i << ": " << colDef << std::endl;
            
            std::istringstream cd(colDef);
            std::string cname, ctype;
            cd >> cname >> ctype;
            int len = -1;
            if (ctype.find("VARCHAR") != std::string::npos)
            {
                size_t l = ctype.find("("), r = ctype.find(")");
                if (l != std::string::npos && r != std::string::npos) {
                    len = std::stoi(ctype.substr(l + 1, r - l - 1));
                    ctype = "VARCHAR";
                }
            }
            std::cout << "[SQLImporter] 解析列: " << cname << " " << ctype << "(" << len << ")" << std::endl;
            ct.table_columns.push_back({cname, ctype, len});
        }

        // 执行创建
        try {
            auto result = exec_->execute(&ct);
            // CREATE TABLE操作可能不返回结果行，这是正常的
            std::cout << "[SQLImporter] 表 " << tableName << " 创建成功" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "[SQLImporter] 创建表失败: " << tableName << " - " << e.what() << std::endl;
            return false;
        }

        // 确保 Catalog 页写入，并更新 MetaPage.catalog_root
        catalog_->SaveToStorage();

        return true;
    }

    // ======= INSERT INTO =======
    bool SQLImporter::HandleInsert(const std::string &sql)
    {
        std::cout << "[SQLImporter] 开始处理INSERT语句: " << sql << std::endl;
        
        // 支持两种格式：
        // INSERT INTO table VALUES('1','Alice','20')
        // INSERT INTO table (col1, col2, col3) VALUES (1, 'Alice', 20)
        size_t p1 = sql.find("INTO");
        size_t p2 = sql.find("VALUES");
        if (p1 == std::string::npos || p2 == std::string::npos) {
            std::cerr << "[SQLImporter] 无法解析INSERT语句格式" << std::endl;
            return false;
        }

        // 提取表名
        std::string tableName = sql.substr(p1 + 5, p2 - (p1 + 5));
        // 去掉列名部分（如果有的话）
        size_t paren_pos = tableName.find('(');
        if (paren_pos != std::string::npos) {
            tableName = tableName.substr(0, paren_pos);
        }
        tableName.erase(tableName.find_last_not_of(" \t") + 1);
        
        std::cout << "[SQLImporter] 解析出表名: " << tableName << std::endl;

        // 提取VALUES部分
        std::string values = sql.substr(p2 + 6);
        // 去掉前导空白
        while (!values.empty() && std::isspace(static_cast<unsigned char>(values.front()))) values.erase(values.begin());
        // 去掉起始括号
        if (!values.empty() && values.front() == '(')
            values.erase(values.begin());
        // 连续去掉末尾的 ")" 和 ";"（例如 ") ;" 或 ");"）
        while (!values.empty() && (values.back() == ')' || values.back() == ';' || std::isspace(static_cast<unsigned char>(values.back()))))
            values.pop_back();

        // 解析值
        std::cout << "[SQLImporter] 解析VALUES部分: " << values << std::endl;
        std::istringstream vs(values);
        std::string val;
        std::vector<std::string> row;
        while (std::getline(vs, val, ','))
        {
            // 去掉引号
            val.erase(std::remove(val.begin(), val.end(), '\''), val.end());
            // 去掉首尾空格
            val.erase(0, val.find_first_not_of(" \t"));
            // 去掉尾部可能残留的 ")" 和 ";"
            while (!val.empty() && (val.back() == ')' || val.back() == ';' || std::isspace(static_cast<unsigned char>(val.back()))))
                val.pop_back();
            val.erase(val.find_last_not_of(" \t") + 1);
            row.push_back(val);
        }
        
        std::cout << "[SQLImporter] 解析出 " << row.size() << " 个值" << std::endl;

        // 构建 PlanNode
        std::cout << "[SQLImporter] 开始构建PlanNode" << std::endl;
        PlanNode ins;
        ins.type = PlanType::Insert;
        ins.table_name = tableName;

        // 读取表结构
        std::cout << "[SQLImporter] 获取表结构: " << tableName << std::endl;
        TableSchema schema = catalog_->GetTable(tableName);
        std::cout << "[SQLImporter] 表结构获取成功，列数: " << schema.columns.size() << std::endl;
        
        // 打印表结构的详细信息
        for (size_t i = 0; i < schema.columns.size(); i++) {
            std::cout << "[SQLImporter] 列 " << i << ": " << schema.columns[i].name 
                      << " " << schema.columns[i].type << "(" << schema.columns[i].length << ")" << std::endl;
        }
        
        ins.columns.clear();
        for (auto &col : schema.columns)
        {
            ins.columns.push_back(col.name);
        }
        ins.values.push_back(row);
        
        std::cout << "[SQLImporter] PlanNode构建完成，准备执行" << std::endl;

        // ===== 关键：先分配数据页（如果没有） =====
        page_id_t first_page_id = schema.first_page_id;
        // Page *cur_page = nullptr;
        if (first_page_id == INVALID_PAGE_ID)
        {
            // page_id_t new_pid = INVALID_PAGE_ID;
            // cur_page = exec_->GetStorageEngine()->CreateDataPage(&new_pid);
            // if (!cur_page)
            // {
            //     std::cerr << "[SQLImporter] 无法创建数据页" << std::endl;
            //     return false;
            // }
            // schema.first_page_id = new_pid;
            // catalog_->CreateTable(schema.table_name, schema.columns); // 更新 Catalog
            // catalog_->SaveToStorage();
            
            // 如果表没有数据页，说明表结构存在但还没有数据
            // 这种情况下，Executor的插入操作应该会自动处理数据页的创建
            std::cout << "[SQLImporter] 表 " << tableName << " 没有数据页，插入操作将自动创建" << std::endl;
        }

        // 执行插入
        std::cout << "[SQLImporter] 开始执行INSERT操作" << std::endl;
        std::cout << "[SQLImporter] PlanNode信息 - 表名: " << ins.table_name << ", 列数: " << ins.columns.size() << ", 值数: " << ins.values.size() << std::endl;
        
        // PermissionChecker现在应该已经正确设置了
        
        try {
            std::cout << "[SQLImporter] 调用exec_->execute()" << std::endl;
            auto result = exec_->execute(&ins);
            std::cout << "[SQLImporter] exec_->execute() 返回，结果行数: " << result.size() << std::endl;
            // INSERT操作可能不返回结果行，这是正常的
            std::cout << "[SQLImporter] 数据插入成功" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "[SQLImporter] 插入数据失败: " << e.what() << std::endl;
            return false;
        } catch (...) {
            std::cerr << "[SQLImporter] 插入数据时发生未知异常" << std::endl;
            return false;
        }

        return true;
    }

} // namespace minidb
