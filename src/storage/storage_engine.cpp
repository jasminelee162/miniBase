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
          buffer_pool_manager_(std::make_unique<BufferPoolManager>(
              (buffer_pool_size ? buffer_pool_size : GetRuntimeConfig().buffer_pool_pages),
              disk_manager_.get())),
          db_file_(db_file)
    {
        // 应用运行时配置
        buffer_pool_manager_->SetMaxPagesFlushedPerCycle(GetRuntimeConfig().bpm_max_flush_per_cycle);
        buffer_pool_manager_->SetFlushIntervalMs(GetRuntimeConfig().bpm_flush_interval_ms);
        // 关闭自适应缓存扩缩功能（按需固定缓存大小）
        buffer_pool_manager_->EnableAutoResize(false);
        buffer_pool_manager_->EnableReadahead(GetRuntimeConfig().bpm_readahead);
        buffer_pool_manager_->SetReadaheadWindow(GetRuntimeConfig().bpm_readahead_window);
        // 设置默认页面替换策略
        SetReplacementPolicy(DEFAULT_REPLACEMENT_POLICY);
        // 启动后台flush线程
        buffer_pool_manager_->StartBackgroundFlusher();
        // 启动StorageEngine级别的后台flush线程
        StartBackgroundFlush(GetRuntimeConfig().bpm_flush_interval_ms);
        // I/O 线程与批量
        // 目前 DiskManager 在构造时启动 1 worker，可根据配置扩展（简化未动态变更）
    }

    StorageEngine::~StorageEngine()
    {
        Shutdown();
    }
    // 获取一页
    Page *StorageEngine::GetPage(page_id_t page_id)
    {
        Page* page = buffer_pool_manager_->FetchPage(page_id);
        global_log_debug(std::string("[StorageEngine::GetPage] page_id=") + std::to_string(page_id) + (page ? " returned valid" : " returned null"));
        return page;
    }
    // 申请新页
    Page *StorageEngine::CreatePage(page_id_t *page_id)
    {
        Page* page = buffer_pool_manager_->NewPage(page_id);
        if (page) {
            global_log_info(std::string("[StorageEngine::CreatePage] Allocated page_id=") + std::to_string(*page_id));
        }
        // 不设置页面类型，让调用者设置
        return page;
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

    // 便捷：创建数据页并初始化
    Page* StorageEngine::CreateDataPage(page_id_t* page_id)
    {
        Page* p = CreatePage(page_id);
        if (p) {
            InitializeDataPage(p);
            PutPage(*page_id, true);
            p = GetPage(*page_id);
        }
        return p;
    }

    // 便捷：按ID获取数据页（校验类型）
    Page* StorageEngine::GetDataPage(page_id_t page_id)
    {
        Page* p = GetPage(page_id);
        if (!p) return nullptr;
        if (p->GetPageType() != PageType::DATA_PAGE) {
            PutPage(page_id, false);
            return nullptr;
        }
        return p;
    }

    // 初始化索引页
    void StorageEngine::InitializeIndexPage(Page* page)
    {
        if (!page) return;
        page->InitializePage(PageType::INDEX_PAGE);
    }

    // 创建索引页
    Page* StorageEngine::CreateIndexPage(page_id_t* page_id)
    {
        Page* p = CreatePage(page_id);
        if (p) {
            InitializeIndexPage(p);
            PutPage(*page_id, true);
            p = GetPage(*page_id);
        }
        return p;
    }

    // 获取索引页
    Page* StorageEngine::GetIndexPage(page_id_t page_id)
    {
        Page* p = GetPage(page_id);
        if (!p) return nullptr;
        if (p->GetPageType() != PageType::INDEX_PAGE) {
            PutPage(page_id, false);
            return nullptr;
        }
        return p;
    }

    // ===== 元数据管理接口实现 =====
    
    // 获取元数据页（第0页）
    Page* StorageEngine::GetMetaPage()
    {
        return GetPage(0);  // 第0页是元数据页
    }
    
    // 初始化元数据页
    bool StorageEngine::InitializeMetaPage()
    {
        MetaPageData disk_meta;
        disk_meta.magic = 0x4D696E6944425F4DULL; // "MiniDB_M"
        disk_meta.version = 1;
        disk_meta.page_size = PAGE_SIZE;
        disk_meta.next_page_id = 1;  // 从第1页开始分配数据页
        disk_meta.catalog_root = INVALID_PAGE_ID;
        
        return disk_manager_->SetMetaInfo(disk_meta);
    }
    
    // 获取元数据信息
    MetaInfo StorageEngine::GetMetaInfo() const
    {
        MetaInfo meta_info;
        MetaPageData disk_meta;
        
        if (disk_manager_->GetMetaInfo(disk_meta)) {
            meta_info.magic = disk_meta.magic;
            meta_info.version = disk_meta.version;
            meta_info.page_size = disk_meta.page_size;
            meta_info.next_page_id = disk_meta.next_page_id;
            meta_info.catalog_root = disk_meta.catalog_root;
        }
        
        return meta_info;
    }
    
    // 更新元数据信息
    bool StorageEngine::UpdateMetaInfo(const MetaInfo& meta_info)
    {
        MetaPageData disk_meta;
        disk_meta.magic = meta_info.magic;
        disk_meta.version = meta_info.version;
        disk_meta.page_size = meta_info.page_size;
        disk_meta.next_page_id = meta_info.next_page_id;
        disk_meta.catalog_root = meta_info.catalog_root;
        
        return disk_manager_->SetMetaInfo(disk_meta);
    }
    
    // 获取目录页
    Page* StorageEngine::GetCatalogPage()
    {
        MetaInfo meta_info = GetMetaInfo();
        if (meta_info.catalog_root == INVALID_PAGE_ID) return nullptr;
        return GetPage(meta_info.catalog_root);
    }
    
    // 创建目录页
    Page* StorageEngine::CreateCatalogPage()
    {
        page_id_t catalog_page_id = INVALID_PAGE_ID;
        Page* catalog_page = CreatePage(&catalog_page_id);
        global_log_info(std::string("[StorageEngine::CreateCatalogPage] CreatePage returned ") + (catalog_page ? "valid" : "null") + " page_id=" + std::to_string(catalog_page_id));
        if (!catalog_page) return nullptr;
        
        // 初始化目录页
        catalog_page->InitializePage(PageType::CATALOG_PAGE);
        global_log_debug("[StorageEngine::CreateCatalogPage] Initialized page as CATALOG_PAGE");
        
        // 更新元数据中的catalog_root（在元数据缺失时做健壮初始化）
        MetaInfo meta_info = GetMetaInfo();
        if (meta_info.page_size == 0) {
            // 说明此前还没有有效的 meta，被视为首次初始化
            meta_info.magic = META_MAGIC;
            meta_info.version = META_VERSION;
            meta_info.page_size = PAGE_SIZE;
            // next_page_id 按磁盘管理器当前统计来设置
            meta_info.next_page_id = static_cast<page_id_t>(disk_manager_->GetNumPages());
        } else {
            // 确保 next_page_id 不回退
            page_id_t dm_next = static_cast<page_id_t>(disk_manager_->GetNumPages());
            if (dm_next > meta_info.next_page_id) {
                meta_info.next_page_id = dm_next;
            }
        }
        meta_info.catalog_root = catalog_page_id;
        UpdateMetaInfo(meta_info);
        global_log_info(std::string("[StorageEngine::CreateCatalogPage] Updated meta_info.catalog_root=") + std::to_string(catalog_page_id));
        
        // 确保页面被写入磁盘
        PutPage(catalog_page_id, true);
        
        // 直接返回创建的页面，调用者需要自己管理
        return catalog_page;
    }
    
    // 更新目录数据
    bool StorageEngine::UpdateCatalogData(const CatalogData& catalog_data)
    {
        Page* catalog_page = GetCatalogPage();
        if (!catalog_page) return false;
        
        // 清空页内容
        catalog_page->InitializePage(PageType::CATALOG_PAGE);
        
        // 将目录数据写入页中
        if (!catalog_data.data.empty()) {
            char* data = catalog_page->GetData() + PAGE_HEADER_SIZE;
            size_t copy_size = std::min(catalog_data.data.size(), 
                                      static_cast<size_t>(PAGE_SIZE - PAGE_HEADER_SIZE));
            std::memcpy(data, catalog_data.data.data(), copy_size);
        }
        
        PutPage(catalog_page->GetPageId(), true);
        return true;
    }
    
    // 获取目录根页号
    page_id_t StorageEngine::GetCatalogRoot() const
    {
        MetaInfo meta_info = GetMetaInfo();
        return meta_info.catalog_root;
    }
    
    // 设置目录根页号
    bool StorageEngine::SetCatalogRoot(page_id_t catalog_root)
    {
        MetaInfo meta_info = GetMetaInfo();
        meta_info.catalog_root = catalog_root;
        return UpdateMetaInfo(meta_info);
    }
    
    // 获取下一个页号
    page_id_t StorageEngine::GetNextPageId() const
    {
        MetaInfo meta_info = GetMetaInfo();
        return meta_info.next_page_id;
    }
    
    // 设置下一个页号
    bool StorageEngine::SetNextPageId(page_id_t next_page_id)
    {
        MetaInfo meta_info = GetMetaInfo();
        meta_info.next_page_id = next_page_id;
        return UpdateMetaInfo(meta_info);
    }

} // namespace minidb
