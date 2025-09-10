#pragma once
#include "util/config.h"
#include "util/status.h"
#include "storage/page/page.h"
#include "storage/buffer/buffer_pool_manager.h"
#include "storage/page/disk_manager.h"
#include <memory>
#include <vector>
#include <string>
#include <algorithm>
#include <mutex>

namespace minidb
{

    class StorageEngine
    {
    public:
        struct TableSchema
        {
            std::string table_name;
            std::vector<std::string> columns;
        };

        explicit StorageEngine(const std::string &db_file,
                               size_t buffer_pool_size = BUFFER_POOL_SIZE);
        ~StorageEngine();

        // 基础页面操作
        Page *GetPage(page_id_t page_id);
        Page *CreatePage(page_id_t *page_id);
        bool PutPage(page_id_t page_id, bool is_dirty = false);
        bool RemovePage(page_id_t page_id);

        // 批量操作
        std::vector<Page *> GetPages(const std::vector<page_id_t> &page_ids);

        // 系统管理
        void Shutdown();
        void Checkpoint();

        // 性能监控和调优
        void PrintStats() const;
        double GetCacheHitRate() const;
        size_t GetBufferPoolSize() const;
        size_t GetNumReplacements() const;
        size_t GetNumWritebacks() const;
        void SetReplacementPolicy(ReplacementPolicy policy);
        bool AdjustBufferPoolSize(size_t new_size);

        // 页数量
        size_t GetNumPages() const;

        // ===== 新增表 schema 接口 =====
        bool CreateTable(const std::string &table_name, const std::vector<std::string> &columns);
        std::vector<std::string> GetTableColumns(const std::string &table_name);

    private:
        std::unique_ptr<DiskManager> disk_manager_;
        std::unique_ptr<BufferPoolManager> buffer_pool_manager_;

        std::string db_file_;
        std::atomic<bool> is_shutdown_{false};

        std::vector<TableSchema> table_schemas_;
        std::mutex schema_mutex_; // 多线程安全
    };

} // namespace minidb
