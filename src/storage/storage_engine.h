// src/storage/storage_engine.h
#pragma once
#include "util/config.h"
#include "util/status.h"
#include "storage/page/page.h"
#include "storage/buffer/buffer_pool_manager.h"
#include "storage/page/disk_manager.h"
#include <memory>
#include <vector>

namespace minidb {

/**
 * 存储引擎：为上层数据库系统提供统一的存储访问接口
 * 存储模块对外的主要API
 */
class StorageEngine {
public:
    explicit StorageEngine(const std::string& db_file, 
                          size_t buffer_pool_size = BUFFER_POOL_SIZE);
    ~StorageEngine();
    
    // 基础页面操作（对外API）
    Page* GetPage(page_id_t page_id);
    Page* CreatePage(page_id_t* page_id);
    bool PutPage(page_id_t page_id, bool is_dirty = false);
    bool RemovePage(page_id_t page_id);
    
    // 批量操作
    std::vector<Page*> GetPages(const std::vector<page_id_t>& page_ids);
    
    // 系统管理
    void Shutdown();
    void Checkpoint();  // 强制刷新所有脏页到磁盘
    
    // 性能监控和调优
    void PrintStats() const;
    double GetCacheHitRate() const;
    size_t GetBufferPoolSize() const;
    size_t GetNumReplacements() const;
    size_t GetNumWritebacks() const;
    void SetReplacementPolicy(ReplacementPolicy policy);
    
    // 动态调优（创新特性）
    bool AdjustBufferPoolSize(size_t new_size);

private:
    std::unique_ptr<DiskManager> disk_manager_;
    std::unique_ptr<BufferPoolManager> buffer_pool_manager_;
    
    std::string db_file_;
    std::atomic<bool> is_shutdown_{false};
};

}