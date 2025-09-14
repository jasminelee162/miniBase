// src/catalog/catalog.cpp
/**
 * 管理表的元信息（表名、列名、列类型等），并持久化到磁盘
 */

#include "catalog.h"
#include <iostream>
#include <sstream>
#include <cstring> // memset
#include <stdexcept>
#include "../util/config.h"              // PAGE_SIZE 常量（按项目实际路径）
#include "../storage/index/bplus_tree.h" // <-- 必须改成你实际的 B+ 树头文件路径

namespace minidb
{

    Catalog::Catalog(StorageEngine *engine) : storage_engine_(engine)
    {
        if (storage_engine_)
            LoadFromStorage();
    }

    void Catalog::SetStorageEngine(StorageEngine *engine)
    {
        std::lock_guard<std::recursive_mutex> lock(latch_);
        storage_engine_ = engine;
    }

    /// @brief
    /// @param table_name
    /// @param columns
    void Catalog::CreateTable(const std::string &table_name,
                              const std::vector<Column> &columns)
    {
        std::cout << "[CreateTable] 开始创建表: " << table_name << std::endl;
        std::lock_guard<std::recursive_mutex> lock(latch_);

        if (tables_.find(table_name) != tables_.end())
        {
            std::cerr << "[Catalog] 表已存在: " << table_name << std::endl;
            return;
        }

        // ===== 确保 CatalogPage 存在 =====
        Page *catalog_page = storage_engine_->GetCatalogPage();
        if (!catalog_page)
        {
            catalog_page = storage_engine_->CreateCatalogPage();
            if (!catalog_page)
            {
                throw std::runtime_error("CreateTable failed: cannot create CatalogPage");
            }
            std::cout << "[Catalog] 自动创建 CatalogPage (pid="
                      << catalog_page->GetPageId() << ")" << std::endl;

            // 更新 MetaInfo
            MetaInfo meta = storage_engine_->GetMetaInfo();
            meta.catalog_root = catalog_page->GetPageId();
            storage_engine_->UpdateMetaInfo(meta);
        }

        // 构造 schema
        TableSchema schema;
        schema.table_name = table_name;
        schema.columns = columns;

        // 分配首个数据页
        page_id_t pid;
        Page *data_page = storage_engine_->CreatePage(&pid);
        if (!data_page)
            throw std::runtime_error("CreateTable failed: cannot create first data page");

        storage_engine_->InitializeDataPage(data_page);
        storage_engine_->PutPage(pid, true);

        schema.first_page_id = pid;
        tables_[table_name] = schema;

        // ===== 统一使用 SaveToStorage() 更新 CatalogPage =====
        SaveToStorage();

        std::cout << "[CreateTable] 表 " << table_name
                  << " 创建成功，目录已保存，首个数据页 = " << pid << std::endl;
    }

    // CatalogData Catalog::SerializeTables()
    // {
    //     CatalogData cd;
    //     std::ostringstream oss;

    //     for (const auto &[name, schema] : tables_)
    //     {
    //         oss << schema.table_name << "|"
    //             << schema.first_page_id;
    //         for (const auto &col : schema.columns)
    //         {
    //             oss << "|" << col.name << ":" << col.type << ":" << col.length;
    //         }
    //         oss << "\n";
    //     }

    //     std::string tmp = oss.str();
    //     cd.data = std::vector<char>(tmp.begin(), tmp.end());
    //     return cd;
    // }

    // void Catalog::DeserializeTables(const CatalogData &cd)
    // {
    //     tables_.clear();
    //     std::string tmp(cd.data.begin(), cd.data.end());
    //     std::istringstream iss(tmp);

    //     std::string line;

    //     while (std::getline(iss, line))
    //     {
    //         if (line.empty())
    //             continue;

    //         std::istringstream ls(line);
    //         std::string token;

    //         // 先读表名和first_page_id
    //         std::getline(ls, token, '|');
    //         std::string table_name = token;

    //         std::getline(ls, token, '|');
    //         page_id_t first_pid = std::stoi(token);

    //         TableSchema schema;
    //         schema.table_name = table_name;
    //         schema.first_page_id = first_pid;

    //         // 继续解析列
    //         while (std::getline(ls, token, '|'))
    //         {
    //             size_t p1 = token.find(':');
    //             size_t p2 = token.find(':', p1 + 1);
    //             if (p1 == std::string::npos || p2 == std::string::npos)
    //                 continue;

    //             Column col;
    //             col.name = token.substr(0, p1);
    //             col.type = token.substr(p1 + 1, p2 - p1 - 1);
    //             col.length = std::stoi(token.substr(p2 + 1));
    //             schema.columns.push_back(col);
    //         }

    //         tables_[table_name] = schema;
    //     }
    // }

    bool Catalog::HasTable(const std::string &table_name) const
    {
        std::lock_guard<std::recursive_mutex> guard(latch_);
        bool found = (tables_.find(table_name) != tables_.end());
        std::cout << "[Catalog::HasTable] table=" << table_name << " found=" << (found ? "yes" : "no") << std::endl;
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

    // ================= 序列化并写入 CatalogPage =================
    void Catalog::SaveToStorage()
    {
        std::lock_guard<std::recursive_mutex> lock(latch_);

        Page *catalog_page = storage_engine_->GetCatalogPage();
        if (!catalog_page)
        {
            throw std::runtime_error("[Catalog::SaveToStorage] catalog 页获取失败");
        }

        std::ostringstream oss;
        for (const auto &[name, schema] : tables_)
        {
            oss << schema.table_name << "|"
                << schema.first_page_id;
            for (const auto &col : schema.columns)
            {
                oss << "|" << col.name << ":" << col.type << ":" << col.length;
            }
            oss << "\n";
        }

        std::string tmp = oss.str();
        std::vector<char> data(tmp.begin(), tmp.end());

        catalog_page->InitializePage(PageType::CATALOG_PAGE);
        char *page_data = catalog_page->GetData() + PAGE_HEADER_SIZE;
        size_t copy_size = std::min(data.size(), static_cast<size_t>(PAGE_SIZE - PAGE_HEADER_SIZE));
        std::memcpy(page_data, data.data(), copy_size);

        storage_engine_->PutPage(catalog_page->GetPageId(), true);
        std::cout << "[Catalog::SaveToStorage] 成功写入目录，共 "
                  << tables_.size() << " 张表" << std::endl;
    }

    // ================= 从 CatalogPage 加载并反序列化 =================
    void Catalog::LoadFromStorage()
    {
        std::lock_guard<std::recursive_mutex> lock(latch_);

        Page *catalog_page = storage_engine_->GetCatalogPage();
        if (!catalog_page)
        {
            std::cerr << "[Catalog::LoadFromStorage] catalog 页不存在" << std::endl;
            return;
        }

        tables_.clear();

        char *page_data = catalog_page->GetData() + PAGE_HEADER_SIZE;
        size_t data_size = PAGE_SIZE - PAGE_HEADER_SIZE;

        // 去掉尾部 \0
        std::string tmp(page_data, strnlen(page_data, data_size));
        std::istringstream iss(tmp);

        std::string line;
        while (std::getline(iss, line))
        {
            if (line.empty())
                continue;

            std::istringstream ls(line);
            std::string token;

            // 表名
            std::getline(ls, token, '|');
            if (token.empty())
                continue;
            std::string table_name = token;

            // first_page_id
            if (!std::getline(ls, token, '|') || token.empty())
            {
                std::cerr << "[Catalog::LoadFromStorage] 缺少 first_page_id, line=" << line << std::endl;
                continue;
            }

            page_id_t first_pid = static_cast<page_id_t>(std::stoi(token));

            TableSchema schema;
            schema.table_name = table_name;
            schema.first_page_id = first_pid;

            // 列信息
            while (std::getline(ls, token, '|'))
            {
                size_t p1 = token.find(':');
                size_t p2 = token.find(':', p1 + 1);
                if (p1 == std::string::npos || p2 == std::string::npos)
                    continue;

                Column col;
                col.name = token.substr(0, p1);
                col.type = token.substr(p1 + 1, p2 - p1 - 1);
                try
                {
                    col.length = std::stoi(token.substr(p2 + 1));
                }
                catch (...)
                {
                    col.length = 0;
                }
                schema.columns.push_back(col);
            }

            tables_[table_name] = schema;
        }

        std::cout << "[Catalog::LoadFromStorage] 加载完成，共 "
                  << tables_.size() << " 张表" << std::endl;
    }

    std::vector<std::string> Catalog::GetTableColumns(const std::string &table_name)
    {
        std::lock_guard<std::recursive_mutex> guard(latch_);
        std::cout << "[Catalog::GetTableColumns] tables_.size=" << tables_.size() << " looking for: " << table_name << std::endl;
        auto it = tables_.find(table_name);
        if (it != tables_.end())
        {
            std::vector<std::string> cols;
            for (auto &c : it->second.columns)
            {
                std::cout << "[Catalog::GetTableColumns] col=" << c.name << std::endl;
                cols.push_back(c.name);
            }
            return cols;
        }
        std::cout << "[Catalog::GetTableColumns] table not found: " << table_name << std::endl;
        return {};
    }

    void Catalog::CreateIndex(const std::string &index_name,
                              const std::string &table_name,
                              const std::vector<std::string> &cols,
                              const std::string &type)
    {
        std::lock_guard<std::recursive_mutex> lock(latch_);

        if (indexes_.find(index_name) != indexes_.end())
        {
            std::cerr << "[Catalog] 索引已存在: " << index_name << std::endl;
            return;
        }
        if (tables_.find(table_name) == tables_.end())
        {
            throw std::runtime_error("[Catalog] 创建索引失败，表不存在: " + table_name);
        }

        IndexSchema idx;
        idx.index_name = index_name;
        idx.table_name = table_name;
        idx.cols = cols;
        idx.type = type;

        if (type == "BPLUS")
        {
            if (!storage_engine_)
                throw std::runtime_error("[Catalog] CreateIndex: StorageEngine 未设置 (需要用于分配 B+ 树页)");

            // 创建 B+ 树，拿到 root page
            BPlusTree bpt(storage_engine_);
            idx.root_page_id = bpt.CreateNew();
        }

        indexes_[index_name] = idx;

        // 持久化
        if (storage_engine_)
            SaveToStorage();
    }

    bool Catalog::HasIndex(const std::string &index_name) const
    {
        std::lock_guard<std::recursive_mutex> lock(latch_);
        return indexes_.find(index_name) != indexes_.end();
    }

    IndexSchema Catalog::GetIndex(const std::string &index_name) const
    {
        std::lock_guard<std::recursive_mutex> lock(latch_);
        auto it = indexes_.find(index_name);
        if (it == indexes_.end())
            throw std::runtime_error("[Catalog] 索引不存在: " + index_name);
        return it->second;
    }

    std::vector<IndexSchema> Catalog::GetTableIndexes(const std::string &table_name) const
    {
        std::vector<IndexSchema> result;
        std::lock_guard<std::recursive_mutex> guard(latch_);
        for (const auto &[name, idx] : indexes_)
        {
            if (idx.table_name == table_name)
            {
                result.push_back(idx);
            }
        }
        return result;
    }

    // Catalog.cpp
    std::string Catalog::FindIndexByColumn(const std::string &table_name, const std::string &col) const
    {
        for (const auto &kv : indexes_)
        {
            const auto &idx = kv.second;
            if (idx.table_name == table_name)
            {
                for (const auto &c : idx.cols)
                {
                    if (c == col)
                    {
                        return idx.index_name;
                    }
                }
            }
        }
        return "";
    }

} // namespace minidb
