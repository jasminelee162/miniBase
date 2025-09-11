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
#include <thread>
#include <atomic>

namespace minidb
{

    class StorageEngine
    {
    public:
        // 移除TableSchema - 这应该由Catalog模块管理

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

        // 后台刷盘控制
        void StartBackgroundFlush(uint64_t interval_ms = 1000);
        void StopBackgroundFlush();

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

        // 页链接操作（这是Storage的核心职责）
        bool LinkPages(page_id_t from_page_id, page_id_t to_page_id);
        
        // ===== 便利性接口（为其他模块提供便利） =====
        // 页遍历工具
        std::vector<Page*> GetPageChain(page_id_t first_page_id);
        // 预取页链（将链上一批页加载到缓冲池，不返回指针）
        void PrefetchPageChain(page_id_t first_page_id, size_t max_pages = 8);
        
        // 页内数据操作工具（使用page_utils.h中的函数）
        bool AppendRecordToPage(Page* page, const void* record_data, uint16_t record_size);
        std::vector<std::pair<const void*, uint16_t>> GetPageRecords(Page* page);
        
        // 页初始化工具
        void InitializeDataPage(Page* page);

    private:
        std::unique_ptr<DiskManager> disk_manager_;
        std::unique_ptr<BufferPoolManager> buffer_pool_manager_;

        std::string db_file_;
        std::atomic<bool> is_shutdown_{false};

        // 后台刷盘
        std::atomic<bool> bg_flush_running_{false};
        std::thread bg_flush_thread_;
        std::atomic<uint64_t> bg_flush_interval_ms_{1000};

        // 移除表schema管理 - 这应该由Catalog模块负责
    };

} // namespace minidb
