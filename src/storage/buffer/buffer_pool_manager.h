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

    // 核心页面操作   缓存池核心接口 
    //  
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
    size_t GetNumReplacements() const { return num_replacements_.load(); }
    size_t GetNumWritebacks() const { return num_writebacks_.load(); }
    void SetPolicy(ReplacementPolicy p) { policy_ = p; }
    
    // 高级特性：动态调整
    bool ResizePool(size_t new_size);

private:
    // 辅助方法
    frame_id_t FindVictimFrame();
    bool FlushFrameToPages(frame_id_t frame_id);
    
    size_t pool_size_;
    Page* pages_;  // 页面池数组
    DiskManager* disk_manager_;
    
    // 页表：page_id -> frame_id 映射 某页在缓存的哪个“槽位”,即帧frame
    std::unordered_map<page_id_t, frame_id_t> page_table_;
    // 反向映射：frame_id -> page_id（用于判定槽位是否占用、写回等）
    std::vector<page_id_t> frame_page_ids_;
    
    // 空闲槽位列表
    std::list<frame_id_t> free_list_;
    
    // LRU替换器
    std::unique_ptr<LRUReplacer> lru_replacer_;
    // FIFO替换器
    std::unique_ptr<FIFOReplacer> fifo_replacer_;
    // 替换策略 默认是LRU
    ReplacementPolicy policy_{ReplacementPolicy::LRU};
    
    // 性能统计  计数器：命中率、访问次数、替换次数、写回次数
    std::atomic<size_t> num_hits_{0};
    std::atomic<size_t> num_accesses_{0};
    std::atomic<size_t> num_replacements_{0};
    std::atomic<size_t> num_writebacks_{0};
    
    // 并发控制
    mutable std::shared_mutex latch_;  // 保护页表和空闲列表
    std::mutex free_list_mutex_;       // 保护空闲列表

    // 仅用于渐进扩容时的新页数组与迁移
    bool GrowPool(size_t new_size);
};

}