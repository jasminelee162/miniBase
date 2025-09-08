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

}