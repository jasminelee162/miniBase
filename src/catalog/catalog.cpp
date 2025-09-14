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
            LoadFromStorage(storage_engine_);
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
            std::cout << "[Catalog] 自动创建 CatalogPage (pid=" << catalog_page->GetPageId() << ")" << std::endl;

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

        // ===== 更新 CatalogPage =====
        CatalogData cd = SerializeTables(); // 序列化内存目录
        catalog_page->InitializePage(PageType::CATALOG_PAGE);
        char *page_data = catalog_page->GetData() + PAGE_HEADER_SIZE;
        size_t copy_size = std::min(cd.data.size(), static_cast<size_t>(PAGE_SIZE - PAGE_HEADER_SIZE));
        std::memcpy(page_data, cd.data.data(), copy_size);
        storage_engine_->PutPage(catalog_page->GetPageId(), true);

        std::cout << "[CreateTable] 表 " << table_name
                  << " 创建成功，目录已写入 CatalogPage，首个数据页 = " << pid << std::endl;
    }

    CatalogData Catalog::SerializeTables()
    {
        CatalogData cd;
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
        cd.data = std::vector<char>(tmp.begin(), tmp.end());
        return cd;
    }

    void Catalog::DeserializeTables(const CatalogData &cd)
    {
        tables_.clear();
        std::string tmp(cd.data.begin(), cd.data.end());
        std::istringstream iss(tmp);

        std::string line;

        while (std::getline(iss, line))
        {
            if (line.empty())
                continue;

            std::istringstream ls(line);
            std::string token;

            // 先读表名和first_page_id
            std::getline(ls, token, '|');
            std::string table_name = token;

            std::getline(ls, token, '|');
            page_id_t first_pid = std::stoi(token);

            TableSchema schema;
            schema.table_name = table_name;
            schema.first_page_id = first_pid;

            // 继续解析列
            while (std::getline(ls, token, '|'))
            {
                size_t p1 = token.find(':');
                size_t p2 = token.find(':', p1 + 1);
                if (p1 == std::string::npos || p2 == std::string::npos)
                    continue;

                Column col;
                col.name = token.substr(0, p1);
                col.type = token.substr(p1 + 1, p2 - p1 - 1);
                col.length = std::stoi(token.substr(p2 + 1));
                schema.columns.push_back(col);
            }

            tables_[table_name] = schema;
        }
    }

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

    void Catalog::LoadFromStorage(StorageEngine *engine)
    {
        std::lock_guard<std::recursive_mutex> lock(latch_);
        if (engine == nullptr)
            engine = storage_engine_;
        if (!engine)
            throw std::runtime_error("[Catalog] LoadFromStorage: StorageEngine 未设置");

        // ---------- 1. 读取 Meta 页 (page0) ----------
        page_id_t meta_pid = 0;
        Page *meta_page = engine->GetPage(meta_pid);
        if (!meta_page)
            throw std::runtime_error("[Catalog::LoadFromStorage] meta page0 不存在，数据库文件可能未初始化");

        // 从 meta_page 读取 catalog_root
        page_id_t catalog_pid;
        std::memcpy(&catalog_pid, meta_page->GetData() + 16, sizeof(page_id_t)); // 约定 offset=16 存 catalog_root

        if (catalog_pid == INVALID_PAGE_ID)
        {
            // 数据库刚初始化，没有目录
            tables_.clear();
            indexes_.clear();
            engine->PutPage(meta_pid, false);
            return;
        }

        // ---------- 2. 读取 Catalog 页 ----------
        Page *catalog_page = engine->GetPage(catalog_pid);
        if (!catalog_page)
        {
            std::cerr << "[Catalog::LoadFromStorage] catalog 页 " << catalog_pid << " 不存在" << std::endl;
            engine->PutPage(meta_pid, false);
            return;
        }

        std::string content(catalog_page->GetData(), PAGE_SIZE);
        std::istringstream iss(content);
        std::string line;

        tables_.clear();
        indexes_.clear();

        while (std::getline(iss, line))
        {
            if (line.empty())
                continue;

            std::istringstream ls(line);
            std::string tag;
            ls >> tag;

            if (tag == "#TABLE")
            {
                TableSchema schema;
                ls >> schema.table_name;

                std::string coldef;
                while (ls >> coldef)
                {
                    Column col;
                    size_t p1 = coldef.find(':');
                    size_t p2 = coldef.find(':', p1 + 1);
                    if (p1 != std::string::npos && p2 != std::string::npos)
                    {
                        col.name = coldef.substr(0, p1);
                        col.type = coldef.substr(p1 + 1, p2 - p1 - 1);
                        col.length = std::stoi(coldef.substr(p2 + 1));
                    }
                    else
                    {
                        col.name = coldef;
                        col.type = "TEXT";
                        col.length = 0;
                    }
                    schema.columns.push_back(col);
                }
                tables_[schema.table_name] = schema;
            }
            else if (tag == "#INDEX")
            {
                IndexSchema idx;
                ls >> idx.index_name >> idx.table_name >> idx.type >> idx.root_page_id;
                std::string colname;
                while (ls >> colname)
                    idx.cols.push_back(colname);
                indexes_[idx.index_name] = idx;
            }
        }

        // ---------- 3. 释放页 ----------
        engine->PutPage(catalog_pid, false);
        engine->PutPage(meta_pid, false);
    }

    void Catalog::SaveToStorage(StorageEngine *engine)
    {
        std::lock_guard<std::recursive_mutex> lock(latch_);
        if (engine == nullptr)
            engine = storage_engine_;
        if (!engine)
            throw std::runtime_error("[Catalog] SaveToStorage: StorageEngine 未设置");

        // ---------- 1. 获取 meta 页 ----------
        page_id_t meta_pid = 0;
        Page *meta_page = engine->GetPage(meta_pid);
        if (!meta_page)
            throw std::runtime_error("[Catalog::SaveToStorage] meta page0 不存在");

        // 从 meta_page 读取 catalog_root
        page_id_t catalog_pid;
        std::memcpy(&catalog_pid, meta_page->GetData() + 16, sizeof(page_id_t));

        // 如果还没分配 catalog 页，就创建一个
        if (catalog_pid == INVALID_PAGE_ID)
        {
            Page *catalog_page_new = engine->CreatePage(&catalog_pid);
            if (!catalog_page_new)
                throw std::runtime_error("[Catalog::SaveToStorage] 无法创建 catalog 页");
            engine->PutPage(catalog_pid, false);

            // 更新 meta_page 中的 catalog_root
            std::memcpy(meta_page->GetData() + 16, &catalog_pid, sizeof(page_id_t));
            engine->PutPage(meta_pid, true); // 标脏
        }
        else
        {
            engine->PutPage(meta_pid, false); // 用完 meta_page
        }

        // ---------- 2. 获取 catalog 页 ----------
        Page *catalog_page = engine->GetPage(catalog_pid);
        if (!catalog_page)
            throw std::runtime_error("[Catalog::SaveToStorage] catalog 页获取失败");

        std::ostringstream oss;
        for (const auto &kv : tables_)
        {
            const TableSchema &schema = kv.second;
            oss << "#TABLE " << schema.table_name;
            for (const auto &col : schema.columns)
                oss << " " << col.name << ":" << col.type << ":" << col.length;
            oss << "\n";
        }
        for (const auto &kv : indexes_)
        {
            const IndexSchema &idx = kv.second;
            oss << "#INDEX " << idx.index_name << " " << idx.table_name
                << " " << idx.type << " " << idx.root_page_id;
            for (const auto &col : idx.cols)
                oss << " " << col;
            oss << "\n";
        }

        std::string s = oss.str();
        if (s.size() > PAGE_SIZE)
            throw std::runtime_error("[Catalog::SaveToStorage] 元数据超出 PAGE_SIZE");

        // 清空旧内容并写入
        std::memset(catalog_page->GetData(), 0, PAGE_SIZE);
        std::memcpy(catalog_page->GetData(), s.data(), s.size());

        // ---------- 3. 标脏并释放 ----------
        engine->PutPage(catalog_pid, true);
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
            SaveToStorage(storage_engine_);
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
