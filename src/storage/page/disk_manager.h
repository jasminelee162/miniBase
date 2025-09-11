// src/storage/page/disk_manager.h
#pragma once
#include "util/config.h"
#include "storage/page/page_header.h"
#include "util/status.h"
#include <string>
#include <fstream>
#include <mutex>
#include <atomic>
#include <future>
#include <vector>
#include <queue>
#include <cstdint>

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
    size_t GetNumPages() const { return next_page_id_.load(); }
    size_t GetNumReads() const { return num_reads_.load(); }
    size_t GetNumWrites() const { return num_writes_.load(); }
    
    // 系统管理
    void FlushAllPages();
    void Shutdown();

    // Meta superblock persistence (page 0)
    bool PersistMeta();

    // 容量信息
    size_t GetMaxPageCount() const { return max_pages_; }
    double GetUsage() const { return GetMaxPageCount() == 0 ? 0.0 : static_cast<double>(next_page_id_.load()) / static_cast<double>(GetMaxPageCount()); }

private:
    // ===== Meta superblock (page 0) =====
    struct MetaPageData {
        uint64_t magic;         // identify valid database file
        uint32_t version;       // meta layout version
        uint32_t page_size;     // sanity check
        uint32_t next_page_id;  // allocated pages count (data pages start from 1)
        uint32_t catalog_root;  // reserved for future catalog root
        uint8_t  reserved[64];  // small padding for future use
    };
    static_assert(PAGE_HEADER_SIZE + sizeof(MetaPageData) <= PAGE_SIZE, "Meta payload exceeds page size");
    static constexpr uint64_t META_MAGIC = 0x4D696E6944425F4DULL; // "MiniDB_M"
    static constexpr uint32_t META_VERSION = 1;
    bool ReadMeta(MetaPageData& out);
    bool WriteMeta(const MetaPageData& m);
    bool InitNewMeta();
    bool LoadOrRecoverMeta();
    size_t GetFileOffset(page_id_t page_id) const {
        return static_cast<size_t>(page_id) * PAGE_SIZE;
    }
    
    std::string db_file_;
    std::fstream file_stream_;
    std::atomic<page_id_t> next_page_id_{0};  // 页面ID从0开始，值即为当前总页数/下一个可用页号
    // 简单空闲页管理：释放的页可复用
    std::queue<page_id_t> free_page_ids_;
    
    // 统计信息
    std::atomic<size_t> num_reads_{0};
    std::atomic<size_t> num_writes_{0};
    
    // 并发控制
    mutable std::mutex file_mutex_;
    std::atomic<bool> is_shutdown_{false};
    size_t max_pages_{DEFAULT_MAX_PAGES};
};

}