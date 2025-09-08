// src/storage/buffer/lru_replacer.h  
#pragma once
#include "util/config.h"
#include <list>
#include <unordered_map>
#include <mutex>

namespace minidb {

class LRUReplacer {
public:
    explicit LRUReplacer(size_t num_pages);
    ~LRUReplacer() = default;
    
    // 不可拷贝
    LRUReplacer(const LRUReplacer&) = delete;
    LRUReplacer& operator=(const LRUReplacer&) = delete;
    
    // 核心接口
    bool Victim(frame_id_t* frame_id);
    void Pin(frame_id_t frame_id);
    void Unpin(frame_id_t frame_id);
    
    // 状态查询
    size_t Size() const;

private:
    size_t capacity_;
    std::list<frame_id_t> lru_list_;
    std::unordered_map<frame_id_t, std::list<frame_id_t>::iterator> lru_map_;
    mutable std::mutex mutex_;
};

class FIFOReplacer {
public:
    explicit FIFOReplacer(size_t num_pages) : capacity_(num_pages) {}
    ~FIFOReplacer() = default;
    FIFOReplacer(const FIFOReplacer&) = delete;
    FIFOReplacer& operator=(const FIFOReplacer&) = delete;

    bool Victim(frame_id_t* frame_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) return false;
        if (queue_.size() < capacity_) return false;
        frame_id_t fid = queue_.front();
        queue_.pop_front();
        in_queue_.erase(fid);
        *frame_id = fid;
        return true;
    }

    void Pin(frame_id_t frame_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = in_queue_.find(frame_id);
        if (it != in_queue_.end()) {
            queue_.erase(it->second);
            in_queue_.erase(it);
        }
    }

    void Unpin(frame_id_t frame_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (in_queue_.find(frame_id) != in_queue_.end()) return;
        queue_.push_back(frame_id);
        in_queue_[frame_id] = std::prev(queue_.end());
    }

    size_t Size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

private:
    size_t capacity_;
    std::list<frame_id_t> queue_;
    std::unordered_map<frame_id_t, std::list<frame_id_t>::iterator> in_queue_;
    mutable std::mutex mutex_;
};

}