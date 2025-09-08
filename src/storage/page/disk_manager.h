// src/storage/page/disk_manager.h
#pragma once
#include "util/config.h"
#include "util/status.h"
#include <string>
#include <fstream>
#include <mutex>
#include <atomic>
#include <future>

namespace minidb {

class DiskManager {
public:
    explicit DiskManager(const std::string& db_file);
    ~DiskManager();
    
    // 同步I/O
    Status ReadPage(page_id_t page_id, char* page_data);
    Status WritePage(page_id_t page_id, const char* page_data);
    
    // 异步I/O (高级特性)
    std::future<Status> ReadPageAsync(page_id_t page_id, char* page_data);
    std::future<Status> WritePageAsync(page_id_t page_id, const char* page_data);
    
    // 页面分配
    page_id_t AllocatePage();
    void DeallocatePage(page_id_t page_id);
    
    // 统计信息
    size_t GetNumPages() const { return next_page_id_.load() - 1; }
    size_t GetNumReads() const { return num_reads_.load(); }
    size_t GetNumWrites() const { return num_writes_.load(); }
    
    // 系统管理
    void FlushAllPages();
    void Shutdown();

private:
    size_t GetFileOffset(page_id_t page_id) const {
        return static_cast<size_t>(page_id) * PAGE_SIZE;
    }
    
    std::string db_file_;
    std::fstream file_stream_;
    std::atomic<page_id_t> next_page_id_{1};  // 页面ID从1开始
    
    // 统计信息
    std::atomic<size_t> num_reads_{0};
    std::atomic<size_t> num_writes_{0};
    
    // 并发控制
    mutable std::mutex file_mutex_;
    std::atomic<bool> is_shutdown_{false};
};

}