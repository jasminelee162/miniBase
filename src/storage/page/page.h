// src/storage/page/page.h
#pragma once
#include "util/config.h"
#include "page_header.h"
#include <atomic>
#include <shared_mutex>
#include <cstring>
#include <cassert>
#include <iostream>

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
    void SetPageId(page_id_t id) { page_id_ = id; }
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
        if (old_count <= 0) {
            // 避免重复释放，但不崩溃程序
            std::cerr << "[Page] Warning: DecPinCount called when pin_count <= 0" << std::endl;
        }
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
    
    // 页头操作方法
    PageHeader* GetHeader() { 
        return reinterpret_cast<PageHeader*>(data_); 
    }
    
    const PageHeader* GetHeader() const { 
        return reinterpret_cast<const PageHeader*>(data_); 
    }
    
    // 页链接操作
    page_id_t GetNextPageId() const {
        return GetHeader()->next_page_id;
    }
    
    void SetNextPageId(page_id_t next_id) {
        GetHeader()->next_page_id = next_id;
    }
    
    // 页类型操作
    PageType GetPageType() const {
        return static_cast<PageType>(GetHeader()->page_type);
    }
    
    void SetPageType(PageType type) {
        GetHeader()->page_type = static_cast<uint32_t>(type);
    }
    
    // 槽管理
    uint16_t GetSlotCount() const {
        return GetHeader()->slot_count;
    }
    
    uint16_t GetFreeSpaceOffset() const {
        return GetHeader()->free_space_offset;
    }
    
    // 初始化新页
    void InitializePage(PageType type = PageType::DATA_PAGE) {
        PageHeader* hdr = GetHeader();
        hdr->slot_count = 0;
        hdr->free_space_offset = PAGE_HEADER_SIZE;
        hdr->next_page_id = INVALID_PAGE_ID;
        hdr->page_type = static_cast<uint32_t>(type);
        hdr->reserved = 0;
        // Header mutated: ensure the page will be flushed
        SetDirty(true);
    }

private:
    alignas(PAGE_SIZE) char data_[PAGE_SIZE];
    page_id_t page_id_{INVALID_PAGE_ID};
    std::atomic<bool> is_dirty_{false};
    std::atomic<int> pin_count_{0};
    mutable std::shared_mutex rwlock_;
};

}