#include "storage/storage_engine.h"
#include "storage/page/page_utils.h"
#include "util/logger.h"
#include "util/config.h"
#include <iostream>
#include <sstream>
#include <unordered_set>
#include <chrono>
#include <thread>

namespace minidb
{

#ifdef PROJECT_ROOT_DIR
    static Logger g_storage_logger_engine(std::string(PROJECT_ROOT_DIR) + "/logs/storage_size.log");
#else
    static Logger g_storage_logger_engine("storage_size.log");
#endif

    StorageEngine::StorageEngine(const std::string &db_file, size_t buffer_pool_size)
        : disk_manager_(std::make_unique<DiskManager>(db_file)),
          buffer_pool_manager_(std::make_unique<BufferPoolManager>(buffer_pool_size, disk_manager_.get())),
          db_file_(db_file)
    {
    }

    StorageEngine::~StorageEngine()
    {
        Shutdown();
    }
    // 获取一页
    Page *StorageEngine::GetPage(page_id_t page_id)
    {
        return buffer_pool_manager_->FetchPage(page_id);
    }
    // 申请新页
    Page *StorageEngine::CreatePage(page_id_t *page_id)
    {
        return buffer_pool_manager_->NewPage(page_id);
    }
    // 用完页后归还缓存，标记脏否
    bool StorageEngine::PutPage(page_id_t page_id, bool is_dirty)
    {
        return buffer_pool_manager_->UnpinPage(page_id, is_dirty);
    }
    // 删除一页
    bool StorageEngine::RemovePage(page_id_t page_id)
    {
        return buffer_pool_manager_->DeletePage(page_id);
    }
    // 获取多页
    std::vector<Page *> StorageEngine::GetPages(const std::vector<page_id_t> &page_ids)
    {
        std::vector<Page *> result;
        result.reserve(page_ids.size());
        for (auto pid : page_ids)
        {
            result.push_back(buffer_pool_manager_->FetchPage(pid));
        }
        return result;
    }
    // 刷脏页并关闭文件
    void StorageEngine::Shutdown()
    {
        if (is_shutdown_.exchange(true))
            return;
        StopBackgroundFlush();
        if (buffer_pool_manager_)
            buffer_pool_manager_->FlushAllPages();
        if (disk_manager_)
            disk_manager_->Shutdown();
    }
    // 只刷脏页不关闭文件
    void StorageEngine::Checkpoint()
    {
        if (buffer_pool_manager_)
            buffer_pool_manager_->FlushAllPages();
        if (disk_manager_)
            disk_manager_->PersistMeta();
    }

    void StorageEngine::StartBackgroundFlush(uint64_t interval_ms)
    {
        bg_flush_interval_ms_.store(interval_ms);
        if (bg_flush_running_.exchange(true)) return;
        bg_flush_thread_ = std::thread([this]() {
            while (bg_flush_running_.load()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(bg_flush_interval_ms_.load()));
                if (!bg_flush_running_.load()) break;
                if (buffer_pool_manager_) buffer_pool_manager_->FlushAllPages();
            }
        });
    }

    void StorageEngine::StopBackgroundFlush()
    {
        if (!bg_flush_running_.exchange(false)) return;
        if (bg_flush_thread_.joinable()) bg_flush_thread_.join();
    }
    // 打印统计信息
    void StorageEngine::PrintStats() const
    {
        std::ostringstream oss;
        oss << "BufferPoolSize=" << GetBufferPoolSize()
            << ", HitRate=" << GetCacheHitRate()
            << ", Replacements=" << GetNumReplacements()
            << ", Writebacks=" << GetNumWritebacks();
        const std::string msg = oss.str();
        std::cout << msg << std::endl;

        if constexpr (ENABLE_STORAGE_LOG) {
            g_storage_logger_engine.log(msg);
        }
    }
    // 缓存命中率
    double StorageEngine::GetCacheHitRate() const
    {
        return buffer_pool_manager_ ? buffer_pool_manager_->GetHitRate() : 0.0;
    }
    // 缓存池大小
    size_t StorageEngine::GetBufferPoolSize() const
    {
        return buffer_pool_manager_ ? buffer_pool_manager_->GetPoolSize() : 0;
    }
    // 调整缓存池大小
    bool StorageEngine::AdjustBufferPoolSize(size_t new_size)
    {
        return buffer_pool_manager_ ? buffer_pool_manager_->ResizePool(new_size) : false;
    }
    // 替换次数
    size_t StorageEngine::GetNumReplacements() const
    {
        return buffer_pool_manager_ ? buffer_pool_manager_->GetNumReplacements() : 0;
    }
    // 写回次数
    size_t StorageEngine::GetNumWritebacks() const
    {
        return buffer_pool_manager_ ? buffer_pool_manager_->GetNumWritebacks() : 0;
    }
    // 设置替换策略
    void StorageEngine::SetReplacementPolicy(ReplacementPolicy policy)
    {
        if (buffer_pool_manager_)
            buffer_pool_manager_->SetPolicy(policy);
    }

    // 页数量
    size_t StorageEngine::GetNumPages() const
    {
        if (!disk_manager_)
            return 0;
        return disk_manager_->GetNumPages();
    }

    // 移除所有表相关操作 - 这些应该由Catalog和Executor模块负责

    // 链接两个页
    bool StorageEngine::LinkPages(page_id_t from_page_id, page_id_t to_page_id)
    {
        // 基本校验：页号必须在已分配范围内
        size_t total_pages = GetNumPages();
        if (from_page_id == INVALID_PAGE_ID || to_page_id == INVALID_PAGE_ID)
            return false;
        if (static_cast<size_t>(from_page_id) >= total_pages)
            return false;
        if (static_cast<size_t>(to_page_id) >= total_pages)
            return false;

        Page* from_page = GetPage(from_page_id);
        if (!from_page) return false;

        from_page->SetNextPageId(to_page_id);
        PutPage(from_page_id, true); // 标记为脏页
        return true;
    }

    // ===== 便利性接口实现 =====
    
    // 页遍历工具：从第一页开始遍历整个页链
    std::vector<Page*> StorageEngine::GetPageChain(page_id_t first_page_id)
    {
        std::vector<Page*> pages;
        page_id_t current_page_id = first_page_id;
        
        // Guard against cycles/self-loops to prevent infinite traversal
        std::unordered_set<page_id_t> visited;
        
        while (current_page_id != INVALID_PAGE_ID) {
            if (visited.find(current_page_id) != visited.end()) {
                break;
            }
            visited.insert(current_page_id);
            Page* page = GetPage(current_page_id);
            if (!page) break; // 获取页失败
            
            pages.push_back(page);
            current_page_id = page->GetNextPageId();
        }
        
        return pages;
    }
    
    void StorageEngine::PrefetchPageChain(page_id_t first_page_id, size_t max_pages)
    {
        page_id_t current_page_id = first_page_id;
        std::unordered_set<page_id_t> visited;
        size_t count = 0;
        while (current_page_id != INVALID_PAGE_ID && count < max_pages) {
            if (visited.find(current_page_id) != visited.end()) break;
            visited.insert(current_page_id);
            Page* page = GetPage(current_page_id);
            if (!page) break;
            // 立即unpin，不标脏，仅将其留在缓冲池（可由替换器决定是否保留）
            PutPage(current_page_id, false);
            current_page_id = page->GetNextPageId();
            ++count;
        }
    }
    
    // 页内数据操作工具：向页追加记录
    bool StorageEngine::AppendRecordToPage(Page* page, const void* record_data, uint16_t record_size)
    {
        if (!page) return false;
        return AppendRow(page, record_data, record_size);
    }
    
    // 页内数据操作工具：获取页内所有记录
    std::vector<std::pair<const void*, uint16_t>> StorageEngine::GetPageRecords(Page* page)
    {
        std::vector<std::pair<const void*, uint16_t>> records;
        if (!page) return records;
        
        ForEachRow(page, [&records](const unsigned char* data, uint16_t len) {
            records.emplace_back(data, len);
        });
        
        return records;
    }
    
    // 页初始化工具：初始化数据页
    void StorageEngine::InitializeDataPage(Page* page)
    {
        if (!page) return;
        page->InitializePage(PageType::DATA_PAGE);
    }

} // namespace minidb
