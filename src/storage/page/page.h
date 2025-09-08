// src/storage/page/page.h
#pragma once
#include "util/config.h"
#include <atomic>
#include <shared_mutex>
#include <cstring>
#include <cassert>

namespace minidb {

class Page {
public:
    Page() { Reset(); }
    explicit Page(page_id_t page_id) : page_id_(page_id) { Reset(); }
    
    // 禁用拷贝，允许移动
    Page(const Page&) = delete;
    Page& operator=(const Page&) = delete;
    Page(Page&&) = default;
    Page& operator=(Page&&) = default;

    // 基础接口
    page_id_t GetPageId() const { return page_id_; }
    char* GetData() { return data_; }
    const char* GetData() const { return data_; }
    
    // 脏页管理
    bool IsDirty() const { return is_dirty_.load(); }
    void SetDirty(bool dirty) { is_dirty_.store(dirty); }
    
    // 引用计数管理
    int GetPinCount() const { return pin_count_.load(); }
    void IncPinCount() { pin_count_.fetch_add(1); }
    void DecPinCount() { 
        int old_count = pin_count_.fetch_sub(1);
        assert(old_count > 0);
    }
    
    // 页面重置
    void Reset() {
        std::memset(data_, 0, PAGE_SIZE);
        page_id_ = INVALID_PAGE_ID;
        is_dirty_.store(false);
        pin_count_.store(0);
    }
    
    // 并发控制
    void RLock() { rwlock_.lock_shared(); }
    void WLock() { rwlock_.lock(); }
    void RUnlock() { rwlock_.unlock_shared(); }
    void WUnlock() { rwlock_.unlock(); }

private:
    alignas(PAGE_SIZE) char data_[PAGE_SIZE];
    page_id_t page_id_{INVALID_PAGE_ID};
    std::atomic<bool> is_dirty_{false};
    std::atomic<int> pin_count_{0};
    mutable std::shared_mutex rwlock_;
};

}