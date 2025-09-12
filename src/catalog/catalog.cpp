// src/catalog/catalog.cpp
/**
 * 管理表的元信息（表名、列名、列类型等），并持久化到磁盘
 */

#include "catalog.h"
#include <iostream>
#include <sstream>

namespace minidb
{

    Catalog::Catalog(const std::string &catalog_file) : catalog_file_(catalog_file)
    {
        Load();
    }

    void Catalog::CreateTable(const std::string &table_name,
                              const std::vector<Column> &columns)
    {
        if (!quiet_) std::cout << "[CreateTable] 开始创建表: " << table_name << std::endl;

        std::lock_guard<std::recursive_mutex> lock(latch_);
<<<<<<< HEAD
        if (!quiet_) std::cout << "[CreateTable] 拿到锁" << std::endl;
=======
>>>>>>> bed7e0f996312fcff4e65fda70d6f4b977f276cb

        if (tables_.find(table_name) != tables_.end())
        {
            if (!quiet_) std::cerr << "[Catalog] 表已存在: " << table_name << std::endl;
            return;
        }

<<<<<<< HEAD
        if (!quiet_) std::cout << "[CreateTable] 构建 schema..." << std::endl;
=======
>>>>>>> bed7e0f996312fcff4e65fda70d6f4b977f276cb
        TableSchema schema;
        schema.table_name = table_name;
        schema.columns = columns; // 直接保存完整列定义

<<<<<<< HEAD
        if (!quiet_) std::cout << "[CreateTable] 插入 tables_ 映射" << std::endl;
        tables_[table_name] = schema;

        if (!quiet_) {
            std::cout << "[CreateTable] columns.size=" << schema.columns.size() << std::endl
                      << std::flush;
        }

        if (!quiet_) std::cout << "[CreateTable] 开始 Save..." << std::endl;
        Save();
        if (!quiet_) std::cout << "[CreateTable] Save 完成" << std::endl;
=======
        tables_[table_name] = schema;
        Save();
>>>>>>> bed7e0f996312fcff4e65fda70d6f4b977f276cb
    }

    bool Catalog::HasTable(const std::string &table_name) const
    {
        std::lock_guard<std::recursive_mutex> guard(latch_);
        bool found = (tables_.find(table_name) != tables_.end());
        if (!quiet_) std::cout << "[Catalog::HasTable] table=" << table_name << " found=" << (found ? "yes" : "no") << std::endl;
        return found;
    }
    TableSchema Catalog::GetTable(const std::string &table_name) const
    {
        std::lock_guard<std::recursive_mutex> lock(latch_);
        auto it = tables_.find(table_name);
        if (it == tables_.end())
        {
            throw std::runtime_error("[Catalog] 表不存在: " + table_name);
        }
        return it->second;
    }

    void Catalog::Load()
    {
        std::lock_guard<std::recursive_mutex> guard(latch_);
        if (loaded_) return; // 避免重复加载
        std::ifstream fin(catalog_file_);
        if (!fin.is_open())
        {
            if (!quiet_) std::cout << "[Load] 文件不存在，跳过加载" << std::endl;
            loaded_ = true;
            return;
        }
        if (!quiet_) std::cout << "[Load] 文件打开成功，开始读取..." << std::endl;

        std::string line;
        while (std::getline(fin, line))
        {
            if (line.empty())
                continue;

            std::istringstream iss(line);
            std::string tbl;
            if (!(iss >> tbl))
                continue;

            TableSchema schema;
            schema.table_name = tbl;

            std::string coldef;
            while (iss >> coldef)
            {
                Column c;
                // 格式: name:type:length
                size_t pos1 = coldef.find(':');
                size_t pos2 = coldef.find(':', pos1 + 1);

                if (pos1 != std::string::npos)
                {
                    c.name = coldef.substr(0, pos1);
                    if (pos2 != std::string::npos)
                    {
                        c.type = coldef.substr(pos1 + 1, pos2 - pos1 - 1);
                        c.length = std::stoi(coldef.substr(pos2 + 1));
                    }
                    else
                    {
                        c.type = coldef.substr(pos1 + 1);
                        c.length = 0;
                    }
                }
                else
                {
                    // 兼容旧格式（只有列名）
                    c.name = coldef;
                    c.type = "TEXT";
                    c.length = 0;
                }

                schema.columns.push_back(c);
            }

            tables_[schema.table_name] = schema;
<<<<<<< HEAD
            if (!quiet_) {
                std::cout << "[Load] 读取行: " << schema.table_name
                          << " cols=" << schema.columns.size() << std::endl;
                std::cout << "[LLLLLLLoad] 读取表: " << schema.table_name
                          << " -> 列数=" << schema.columns.size() << std::endl;
                for (auto &c : schema.columns)
                {
                    std::cout << "   列: " << c.name << ":" << c.type << std::endl;
                }
=======
            std::cout << "[Load] 表: " << schema.table_name
                      << " -> 列数=" << schema.columns.size() << std::endl;
            for (auto &c : schema.columns)
            {
                std::cout << "   列: " << c.name << " " << c.type << "(" << c.length << ")" << std::endl;
>>>>>>> bed7e0f996312fcff4e65fda70d6f4b977f276cb
            }
        }

        fin.close();
        if (!quiet_) std::cout << "[Load] 文件读取完成" << std::endl;
        loaded_ = true;
    }

    void Catalog::Save()
    {
        std::lock_guard<std::recursive_mutex> lock(latch_);
        std::ofstream fout(catalog_file_, std::ios::trunc);
        if (!fout.is_open())
        {
            std::cerr << "[Save] 文件打开失败！" << std::endl;
            return;
        }

        for (const auto &kv : tables_)
        {
            const TableSchema &schema = kv.second;
<<<<<<< HEAD
            if (!quiet_) std::cout << "[SSSSSSSSSave] 写表: " << schema.table_name
                      << " 列数=" << schema.columns.size() << std::endl;
=======
>>>>>>> bed7e0f996312fcff4e65fda70d6f4b977f276cb
            fout << schema.table_name;
            for (const auto &col : schema.columns)
            {
                // 保存格式: name:type:length
                fout << " " << col.name << ":" << col.type << ":" << col.length;
            }
            fout << "\n";
        }
    }

    std::vector<std::string> Catalog::GetTableColumns(const std::string &table_name)
    {
        std::lock_guard<std::recursive_mutex> guard(latch_);
        if (!quiet_) std::cout << "[Catalog::GetTableColumns] tables_.size=" << tables_.size() << " looking for: " << table_name << std::endl;
        auto it = tables_.find(table_name);
        if (it != tables_.end())
        {
            std::vector<std::string> cols;
            for (auto &c : it->second.columns)
            {
                if (!quiet_) std::cout << "[Catalog::GetTableColumns] col=" << c.name << std::endl;
                cols.push_back(c.name);
            }
            return cols;
        }
        if (!quiet_) std::cout << "[Catalog::GetTableColumns] table not found: " << table_name << std::endl;
        return {};
    }

} // namespace minidb
