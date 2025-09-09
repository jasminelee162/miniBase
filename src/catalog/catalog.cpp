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
                              const std::vector<std::string> &columns)
    {
        std::cout << "[CreateTable] 开始创建表: " << table_name << std::endl;

        std::lock_guard<std::recursive_mutex> lock(latch_);
        std::cout << "[CreateTable] 拿到锁" << std::endl;

        if (tables_.find(table_name) != tables_.end())
        {
            std::cerr << "[Catalog] 表已存在: " << table_name << std::endl;
            return;
        }

        std::cout << "[CreateTable] 构建 schema..." << std::endl;
        TableSchema schema;
        schema.table_name = table_name;
        for (const auto &col : columns)
            schema.columns.push_back(Column{col, "TEXT"});

        std::cout << "[CreateTable] 插入 tables_ 映射" << std::endl;
        tables_[table_name] = schema;

        std::cout << "[CreateTable] 开始 Save..." << std::endl;
        Save(); // ← 大概率卡在这里
        std::cout << "[CreateTable] Save 完成" << std::endl;
    }

    bool Catalog::HasTable(const std::string &table_name) const
    {
        std::lock_guard<std::recursive_mutex> lock(latch_);
        return tables_.find(table_name) != tables_.end();
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
        std::lock_guard<std::recursive_mutex> lock(latch_);
        std::cout << "[Load] 开始加载 catalog: " << catalog_file_ << std::endl;
        std::ifstream fin(catalog_file_);
        if (!fin.is_open())
        {
            std::cout << "[Load] 文件不存在，跳过加载" << std::endl;
            return;
        }

        std::cout << "[Load] 文件打开成功，开始读取..." << std::endl;
        tables_.clear();
        std::string line;
        while (std::getline(fin, line))
        {
            std::cout << "[Load] 读取行: " << line << std::endl;
            std::istringstream iss(line);
            std::string table_name;
            iss >> table_name;

            TableSchema schema;
            schema.table_name = table_name;
            std::string col;
            while (iss >> col)
            {
                schema.columns.push_back(Column{col, "TEXT"});
            }
            tables_[table_name] = schema;
        }
        std::cout << "[Load] 文件读取完成" << std::endl;
        fin.close();
    }

    void Catalog::Save()
    {
        std::cout << "[Save] 拿到锁" << std::endl;
        std::lock_guard<std::recursive_mutex> lock(latch_);

        std::cout << "[Save] 打开文件: " << catalog_file_ << std::endl;
        std::ofstream fout(catalog_file_, std::ios::trunc);
        if (!fout.is_open())
        {
            std::cerr << "[Save] 文件打开失败！" << std::endl;
            return;
        }

        std::cout << "[Save] 开始写入..." << std::endl;
        for (const auto &kv : tables_)
        {
            fout << kv.second.table_name;
            for (const auto &col : kv.second.columns)
                fout << " " << col.name;
            fout << "\n";
        }
        std::cout << "[Save] 写入完成，关闭文件" << std::endl;
        fout.close();
    }

} // namespace minidb
