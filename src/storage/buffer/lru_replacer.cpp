// src/storage/buffer/lru_replacer.cpp
#include "lru_replacer.h"
#include <cassert>

namespace minidb {

LRUReplacer::LRUReplacer(size_t num_pages) : capacity_(num_pages) {}

bool LRUReplacer::Victim(frame_id_t* frame_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    // 仅当达到容量上限时才允许淘汰
    if (lru_list_.size() < capacity_) {
        return false;
    }
    if (lru_list_.empty()) {
        return false;
    }
    // 淘汰最久未使用（链表尾部）
    frame_id_t victim = lru_list_.back();
    lru_list_.pop_back();
    lru_map_.erase(victim);
    *frame_id = victim;
    return true;
}

void LRUReplacer::Pin(frame_id_t frame_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = lru_map_.find(frame_id);
    if (it != lru_map_.end()) {
        lru_list_.erase(it->second);
        lru_map_.erase(it);
    }
}

void LRUReplacer::Unpin(frame_id_t frame_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    // 已在 LRU 中则忽略
    if (lru_map_.find(frame_id) != lru_map_.end()) {
        return;
    }
    // 插入到头部表示最新未使用；不在此处做容量裁剪
    lru_list_.push_front(frame_id);
    lru_map_[frame_id] = lru_list_.begin();
}

size_t LRUReplacer::Size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return lru_map_.size();
}

} // namespace minidb