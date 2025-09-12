#include "storage/page/disk_manager.h"
#include "util/logger.h"
#include <cassert>
#include <cstring>
#include <future>
#include <algorithm>
#include "storage/page/page_header.h"
#include "util/config.h"

namespace minidb {

#ifdef PROJECT_ROOT_DIR
static Logger g_storage_logger(std::string(PROJECT_ROOT_DIR) + "/logs/storage.log");
#else
static Logger g_storage_logger("storage.log");
#endif

    // 负责把页号映射到文件偏移，并做读写。内部用互斥锁保证线程安全。

    DiskManager::DiskManager(const std::string &db_file) : db_file_(db_file)
    {
        std::lock_guard<std::mutex> lock(file_mutex_);
        bool newly_preallocated = false;
        file_stream_.open(db_file_, std::ios::in | std::ios::out | std::ios::binary);
        if (!file_stream_.is_open())
        {
            // Create if not exists and preallocate DEFAULT_DISK_SIZE_BYTES
            std::fstream create_stream(db_file_, std::ios::out | std::ios::binary);
            if (create_stream.is_open())
            {
                if (DEFAULT_DISK_SIZE_BYTES > 0)
                {
                    create_stream.seekp(static_cast<std::streamoff>(DEFAULT_DISK_SIZE_BYTES - 1), std::ios::beg);
                    char zero = 0;
                    create_stream.write(&zero, 1);
                    newly_preallocated = true;
                }
                create_stream.close();
            }
            file_stream_.open(db_file_, std::ios::in | std::ios::out | std::ios::binary);
        }
        file_stream_.seekg(0, std::ios::end);
        std::streamoff size = file_stream_.tellg();
        size_t file_pages = 0;
        if (size > 0)
        {
            file_pages = static_cast<size_t>(size) / PAGE_SIZE;
            if (file_pages > 0)
            {
                max_pages_ = std::max(max_pages_, file_pages);
            }
        }
        // 使用超级块（page 0）加载或初始化元数据
        if (!LoadOrRecoverMeta()) {
            next_page_id_.store(0);
        }
    }

    DiskManager::~DiskManager()
    {
        Shutdown();
    }
    // 读，如果超出文件末尾，就返回“全 0”缓冲（对新页或空洞很友好)
    Status DiskManager::ReadPage(page_id_t page_id, char *page_data)
    {
        if (page_id == INVALID_PAGE_ID || page_data == nullptr)
        {
            return Status::INVALID_PARAM;
        }
        std::lock_guard<std::mutex> lock(file_mutex_);
        if (is_shutdown_.load())
        {
            return Status::IO_ERROR;
        }
        size_t offset = GetFileOffset(page_id);
        file_stream_.seekg(0, std::ios::end);
        std::streamoff file_size = file_stream_.tellg();
        if (static_cast<std::streamoff>(offset) >= file_size)
        {
            // Reading beyond EOF: return zero-filled page
            std::memset(page_data, 0, PAGE_SIZE);
            return Status::OK;
        }
        file_stream_.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
        file_stream_.read(page_data, PAGE_SIZE);
        if (file_stream_.gcount() < static_cast<std::streamsize>(PAGE_SIZE))
        {
            // Short read, zero remainder
            std::streamsize read_cnt = file_stream_.gcount();
            std::memset(page_data + read_cnt, 0, PAGE_SIZE - static_cast<size_t>(read_cnt));
        }
        if (!file_stream_)
        {
            return Status::IO_ERROR;
        }
        num_reads_.fetch_add(1);
        if constexpr (ENABLE_STORAGE_LOG) {
            g_storage_logger.log(std::string("[DM] Read page ") + std::to_string((unsigned)page_id));
        }
        return Status::OK;
    }
    // 写，若超出文件末尾，则更新next_page_id_
    Status DiskManager::WritePage(page_id_t page_id, const char *page_data)
    {
        if (page_id == INVALID_PAGE_ID || page_data == nullptr)
        {
            return Status::INVALID_PARAM;
        }
        std::lock_guard<std::mutex> lock(file_mutex_);
        if (is_shutdown_.load())
        {
            return Status::IO_ERROR;
        }
        size_t offset = GetFileOffset(page_id);
        file_stream_.seekp(static_cast<std::streamoff>(offset), std::ios::beg);
        file_stream_.write(page_data, PAGE_SIZE);
        file_stream_.flush();
        if (!file_stream_)
        {
            return Status::IO_ERROR;
        }
        num_writes_.fetch_add(1);
        if constexpr (ENABLE_STORAGE_LOG) {
            g_storage_logger.log(std::string("[DM] Write page ") + std::to_string((unsigned)page_id));
        }
        // 依据写入的页号推进下一个可用页号（0基）：next = max(next, page_id + 1)
        page_id_t expected_next = static_cast<page_id_t>(page_id + 1);
        page_id_t cur = next_page_id_.load();
        if (expected_next > cur)
        {
            next_page_id_.store(expected_next);
        }
        return Status::OK;
    }

    std::future<Status> DiskManager::ReadPageAsync(page_id_t page_id, char *page_data)
    {
        return std::async(std::launch::async, [this, page_id, page_data]()
                          { return ReadPage(page_id, page_data); });
    }

    std::future<Status> DiskManager::WritePageAsync(page_id_t page_id, const char *page_data)
    {
        return std::async(std::launch::async, [this, page_id, page_data]()
                          { return WritePage(page_id, page_data); });
    }
    // 返回新页号（优先复用空闲队列里的)
    page_id_t DiskManager::AllocatePage()
    {
        std::lock_guard<std::mutex> lock(file_mutex_);
        // 先复用空闲页
        if (!free_page_ids_.empty())
        {
            page_id_t pid = free_page_ids_.front();
            free_page_ids_.pop();
            return pid;
        }
        // 检查容量
        page_id_t next = next_page_id_.load();
        if (static_cast<size_t>(next) >= max_pages_) {
            if constexpr (ENABLE_STORAGE_LOG) {
                g_storage_logger.log(Logger::Level::WARN, std::string("[DM] Disk full: next=") + std::to_string((unsigned)next) +
                                   ", max_pages=" + std::to_string(max_pages_));
            }
            return INVALID_PAGE_ID;
        }
        return next_page_id_.fetch_add(1);
    }
    // 释放页号（加入空闲队列)
    void DiskManager::DeallocatePage(page_id_t page_id)
    {
        if (page_id == INVALID_PAGE_ID)
            return;
        std::lock_guard<std::mutex> lock(file_mutex_);
        // 简单复用：加入空闲队列（不做去重，调用方需保证合法性）
        free_page_ids_.push(page_id);
    }
    // 强制将所有缓冲区的数据写会磁盘
    void DiskManager::FlushAllPages()
    {
        std::lock_guard<std::mutex> lock(file_mutex_);
        file_stream_.flush();
    }

    void DiskManager::Shutdown()
    {
        std::lock_guard<std::mutex> lock(file_mutex_);
        if (is_shutdown_.exchange(true))
        {
            return;
        }
        PersistMeta();
        if (file_stream_.is_open())
        {
            file_stream_.flush();
            file_stream_.close();
        }
    }

    bool DiskManager::ReadMeta(MetaPageData& out)
    {
        if (!file_stream_.is_open()) return false;
        std::vector<char> buf(PAGE_SIZE);
        file_stream_.seekg(0, std::ios::beg);
        file_stream_.read(buf.data(), PAGE_SIZE);
        if (!file_stream_) return false;
        // Validate header
        const PageHeader* hdr = reinterpret_cast<const PageHeader*>(buf.data());
        // Accept either METADATA_PAGE or legacy zeroed header
        if (!(hdr->slot_count == 0 && hdr->free_space_offset >= PAGE_HEADER_SIZE)) {
            // If header malformed, still try to read meta region
        }
        std::memcpy(&out, buf.data() + PAGE_HEADER_SIZE, sizeof(MetaPageData));
        if (out.magic != META_MAGIC) return false;
        if (out.version != META_VERSION) return false;
        if (out.page_size != PAGE_SIZE) return false;
        return true;
    }

    bool DiskManager::WriteMeta(const MetaPageData& m)
    {
        if (!file_stream_.is_open()) return false;
        std::vector<char> buf(PAGE_SIZE);
        std::memset(buf.data(), 0, buf.size());
        // Fill header as METADATA_PAGE with zero slots
        PageHeader* hdr = reinterpret_cast<PageHeader*>(buf.data());
        hdr->slot_count = 0;
        hdr->free_space_offset = PAGE_HEADER_SIZE;
        hdr->next_page_id = INVALID_PAGE_ID;
        hdr->page_type = static_cast<uint32_t>(PageType::METADATA_PAGE);
        hdr->reserved = 0;
        // Copy meta payload after header
        std::memcpy(buf.data() + PAGE_HEADER_SIZE, &m, sizeof(MetaPageData));
        file_stream_.seekp(0, std::ios::beg);
        file_stream_.write(buf.data(), PAGE_SIZE);
        file_stream_.flush();
        return static_cast<bool>(file_stream_);
    }

    bool DiskManager::InitNewMeta()
    {
        MetaPageData m{};
        std::memset(&m, 0, sizeof(m));
        m.magic = META_MAGIC;
        m.version = META_VERSION;
        m.page_size = static_cast<uint32_t>(PAGE_SIZE);
        m.next_page_id = 1; // 预留页0
        m.catalog_root = INVALID_PAGE_ID;
        if (!WriteMeta(m)) return false;
        next_page_id_.store(m.next_page_id);
        return true;
    }

    bool DiskManager::LoadOrRecoverMeta()
    {
        MetaPageData m{};
        if (ReadMeta(m)) {
            next_page_id_.store(m.next_page_id);
            return true;
        }
        return InitNewMeta();
    }

    bool DiskManager::PersistMeta()
    {
        MetaPageData m{};
        m.magic = META_MAGIC;
        m.version = META_VERSION;
        m.page_size = static_cast<uint32_t>(PAGE_SIZE);
        m.next_page_id = next_page_id_.load();
        m.catalog_root = INVALID_PAGE_ID;
        return WriteMeta(m);
    }

    // ===== 元数据访问接口实现 =====
    
    bool DiskManager::GetMetaInfo(MetaPageData& out) const
    {
        if (meta_cached_.load()) {
            out = cached_meta_;
            return true;
        }
        
        // 从磁盘读取
        MetaPageData temp;
        if (const_cast<DiskManager*>(this)->ReadMeta(temp)) {
            const_cast<DiskManager*>(this)->cached_meta_ = temp;
            const_cast<DiskManager*>(this)->meta_cached_.store(true);
            out = temp;
            return true;
        }
        return false;
    }
    
    bool DiskManager::SetMetaInfo(const MetaPageData& meta)
    {
        if (!WriteMeta(meta)) return false;
        
        // 更新缓存
        cached_meta_ = meta;
        meta_cached_.store(true);
        
        // 同步next_page_id
        next_page_id_.store(meta.next_page_id);
        
        return true;
    }
    
    page_id_t DiskManager::GetCatalogRoot() const
    {
        MetaPageData meta;
        if (GetMetaInfo(meta)) {
            return meta.catalog_root;
        }
        return INVALID_PAGE_ID;
    }
    
    bool DiskManager::SetCatalogRoot(page_id_t catalog_root)
    {
        MetaPageData meta;
        if (!GetMetaInfo(meta)) return false;
        
        meta.catalog_root = catalog_root;
        return SetMetaInfo(meta);
    }
} // namespace minidb
