// src/storage/buffer/buffer_pool_manager.h
#pragma once
#include "util/config.h"
#include "util/status.h"
#include "storage/page/page.h"
#include "storage/page/disk_manager.h"
#include "storage/buffer/lru_replacer.h"
#include <memory>
#include <unordered_map>
#include <list>
#include <shared_mutex>
#include <atomic>
#include <vector>

namespace minidb {

class BufferPoolManager {
public:
    BufferPoolManager(size_t pool_size, DiskManager* disk_manager);
    ~BufferPoolManager();
    
    // 不可拷贝
    BufferPoolManager(const BufferPoolManager&) = delete;
    BufferPoolManager& operator=(const BufferPoolManager&) = delete;

    // 核心页面操作
    Page* FetchPage(page_id_t page_id);
    Page* NewPage(page_id_t* page_id);
    bool UnpinPage(page_id_t page_id, bool is_dirty);
    bool FlushPage(page_id_t page_id);
    bool DeletePage(page_id_t page_id);
    
    // 批量操作
    void FlushAllPages();
    
    // 性能统计
    double GetHitRate() const;
    size_t GetPoolSize() const { return pool_size_; }
    size_t GetFreeFramesCount() const;
    
    // 高级特性：动态调整
    bool ResizePool(size_t new_size);

private:
    // 辅助方法
    frame_id_t FindVictimFrame();
    bool FlushFrameToPages(frame_id_t frame_id);
    
    size_t pool_size_;
    Page* pages_;  // 页面池数组
    DiskManager* disk_manager_;
    
    // 页表：page_id -> frame_id 映射
    std::unordered_map<page_id_t, frame_id_t> page_table_;
    // 反向映射：frame_id -> page_id（用于判定帧是否占用、写回等）
    std::vector<page_id_t> frame_page_ids_;
    
    // 空闲帧列表
    std::list<frame_id_t> free_list_;
    
    // LRU替换器
    std::unique_ptr<LRUReplacer> replacer_;
    
    // 性能统计
    std::atomic<size_t> num_hits_{0};
    std::atomic<size_t> num_accesses_{0};
    
    // 并发控制
    mutable std::shared_mutex latch_;  // 保护页表和空闲列表
    std::mutex free_list_mutex_;       // 保护空闲列表
};

}