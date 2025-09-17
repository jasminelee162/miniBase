#include "storage/page/disk_manager.h"
#include "util/logger.h"
#include <cassert>
#include <cstring>
#include <future>
#include <algorithm>
#include <iostream>
#include <chrono>
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
        
        // 确保max_pages_至少等于next_page_id_，避免无法分配新页面
        max_pages_ = std::max(max_pages_, static_cast<size_t>(next_page_id_.load() + 100));
        
        global_log_info(std::string("[DiskManager::DiskManager] Initialized next_page_id_=") + std::to_string(next_page_id_.load()) + " (this=" + std::to_string(reinterpret_cast<uintptr_t>(this)) + ")");
        // 根据配置启动N个I/O工作线程，并设置批量大小
        StartWorkers(GetRuntimeConfig().io_worker_threads);
        batch_max_ = GetRuntimeConfig().io_batch_max;
    }

    DiskManager::~DiskManager()
    {
        Shutdown();
        StopWorkers();
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
        // WAL: 先写日志，再写数据
        if (wal_ != nullptr) {
            wal_->Append(page_id, page_data);
        }
        size_t offset = GetFileOffset(page_id);
        file_stream_.seekp(static_cast<std::streamoff>(offset), std::ios::beg);
        file_stream_.write(page_data, PAGE_SIZE);
        file_stream_.flush();
        if (!file_stream_)
        {
            global_log_warn(std::string("[DiskManager::WritePage] Write failed for page_id=") + std::to_string(page_id));
            return Status::IO_ERROR;
        }
        global_log_debug(std::string("[DiskManager::WritePage] Successfully wrote page_id=") + std::to_string(page_id) + ", offset=" + std::to_string(offset));
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
        return Enqueue(IOType::Read, page_id, page_data, nullptr);
    }

    std::future<Status> DiskManager::WritePageAsync(page_id_t page_id, const char *page_data)
    {
        return Enqueue(IOType::Write, page_id, nullptr, page_data);
    }

    void DiskManager::StartWorkers(size_t n)
    {
        stop_workers_.store(false);
        for (size_t i = 0; i < n; ++i) {
            workers_.emplace_back(&DiskManager::WorkerLoop, this);
        }
    }

    void DiskManager::StopWorkers()
    {
        stop_workers_.store(true);
        queue_cv_.notify_all();
        for (auto &t : workers_) {
            if (t.joinable()) t.join();
        }
        workers_.clear();
    }

    std::future<Status> DiskManager::Enqueue(IOType type, page_id_t page_id, char* rbuf, const char* wbuf)
    {
        IORequest req{type, page_id, rbuf, wbuf, std::promise<Status>()};
        std::future<Status> fut = req.prom.get_future();
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            io_queue_.emplace_back(std::move(req));
        }
        queue_cv_.notify_one();
        return fut;
    }

    void DiskManager::WorkerLoop()
    {
        while (!stop_workers_.load()) {
            std::deque<IORequest> batch;
            {
                std::unique_lock<std::mutex> lk(queue_mutex_);
                queue_cv_.wait(lk, [&]{ return stop_workers_.load() || !io_queue_.empty(); });
                if (stop_workers_.load()) break;
                // 批量弹出，最多 batch_max_
                size_t take = std::min(batch_max_, io_queue_.size());
                for (size_t i = 0; i < take; ++i) {
                    batch.emplace_back(std::move(io_queue_.front()));
                    io_queue_.pop_front();
                }
            }
            // 简单排序：Write 按 page_id 升序，Read 保持相对次序
            std::stable_sort(batch.begin(), batch.end(), [](const IORequest& a, const IORequest& b){
                if (a.type != b.type) return a.type == IOType::Write; // 写优先分组
                if (a.type == IOType::Write) return a.page_id < b.page_id; // 写按页顺序
                return false;
            });
            // 逐个执行（可进一步合并相邻页写入）
            for (auto &req : batch) {
                Status s = Status::IO_ERROR;
                auto t0 = std::chrono::high_resolution_clock::now();
                if (req.type == IOType::Read) {
                    s = ReadPage(req.page_id, req.read_buf);
                    read_ops_.fetch_add(1);
                } else {
                    s = WritePage(req.page_id, req.write_buf);
                    write_ops_.fetch_add(1);
                }
                auto t1 = std::chrono::high_resolution_clock::now();
                uint64_t ns = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
                if (req.type == IOType::Read) total_read_ns_.fetch_add(ns); else total_write_ns_.fetch_add(ns);
                req.prom.set_value(s);
            }
        }
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
            global_log_debug(std::string("[DiskManager::AllocatePage] Reusing free page_id=") + std::to_string(pid));
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
        // 使用更显式的原子操作
        page_id_t current = next_page_id_.load();
        global_log_debug(std::string("[DiskManager::AllocatePage] Before fetch_add: next_page_id_=") + std::to_string(current));
        page_id_t allocated = next_page_id_.fetch_add(1);
        page_id_t after = next_page_id_.load();
        global_log_debug(std::string("[DiskManager::AllocatePage] After fetch_add: allocated=") + std::to_string(allocated) + ", next_page_id_=" + std::to_string(after));
        return allocated;
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
        // checksum validate (stored at reserved[8..11])
        uint32_t stored_crc = 0;
        std::memcpy(&stored_crc, out.reserved + 8, sizeof(uint32_t));
        // compute crc over fields except reserved area (simple xor-based checksum)
        uint32_t crc = 0;
        auto mix = [&](const uint8_t* p, size_t n){ for (size_t i=0;i<n;++i) crc = (crc * 16777619u) ^ p[i]; };
        mix(reinterpret_cast<const uint8_t*>(&out.magic), sizeof(out.magic));
        mix(reinterpret_cast<const uint8_t*>(&out.version), sizeof(out.version));
        mix(reinterpret_cast<const uint8_t*>(&out.page_size), sizeof(out.page_size));
        mix(reinterpret_cast<const uint8_t*>(&out.next_page_id), sizeof(out.next_page_id));
        mix(reinterpret_cast<const uint8_t*>(&out.catalog_root), sizeof(out.catalog_root));
        if (stored_crc != 0 && stored_crc != crc) return false;
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
        // Copy meta payload after header, with epoch++ and checksum
        MetaPageData temp = m;
        // epoch stored at reserved[0..7]
        uint64_t epoch = 0;
        std::memcpy(&epoch, temp.reserved + 0, sizeof(uint64_t));
        epoch++;
        std::memcpy(temp.reserved + 0, &epoch, sizeof(uint64_t));
        // checksum at reserved[8..11]
        uint32_t crc = 0;
        auto mix = [&](const uint8_t* p, size_t n){ for (size_t i=0;i<n;++i) crc = (crc * 16777619u) ^ p[i]; };
        mix(reinterpret_cast<const uint8_t*>(&temp.magic), sizeof(temp.magic));
        mix(reinterpret_cast<const uint8_t*>(&temp.version), sizeof(temp.version));
        mix(reinterpret_cast<const uint8_t*>(&temp.page_size), sizeof(temp.page_size));
        mix(reinterpret_cast<const uint8_t*>(&temp.next_page_id), sizeof(temp.next_page_id));
        mix(reinterpret_cast<const uint8_t*>(&temp.catalog_root), sizeof(temp.catalog_root));
        std::memcpy(temp.reserved + 8, &crc, sizeof(uint32_t));
        std::memcpy(buf.data() + PAGE_HEADER_SIZE, &temp, sizeof(MetaPageData));
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
            global_log_info(std::string("[DiskManager::LoadOrRecoverMeta] ReadMeta success, next_page_id=") + std::to_string(m.next_page_id));
            next_page_id_.store(m.next_page_id);
            return true;
        }
        global_log_warn("[DiskManager::LoadOrRecoverMeta] ReadMeta failed, calling InitNewMeta");
        return InitNewMeta();
    }

    bool DiskManager::PersistMeta()
    {
        MetaPageData m{};
        m.magic = META_MAGIC;
        m.version = META_VERSION;
        m.page_size = static_cast<uint32_t>(PAGE_SIZE);
        m.next_page_id = next_page_id_.load();
        // 保持现有的catalog_root，不要重置为INVALID_PAGE_ID
        MetaPageData current_meta;
        if (GetMetaInfo(current_meta)) {
            m.catalog_root = current_meta.catalog_root;
        } else {
            m.catalog_root = INVALID_PAGE_ID;
        }
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

    // 为简化：把 index_root 存放到 reserved[0..3]（little-endian 32bit）
    page_id_t DiskManager::GetIndexRoot() const
    {
        MetaPageData meta;
        if (!GetMetaInfo(meta)) return INVALID_PAGE_ID;
        uint32_t v = 0;
        std::memcpy(&v, meta.reserved + 0, sizeof(uint32_t));
        return static_cast<page_id_t>(v);
    }

    bool DiskManager::SetIndexRoot(page_id_t index_root)
    {
        MetaPageData meta;
        if (!GetMetaInfo(meta)) return false;
        uint32_t v = static_cast<uint32_t>(index_root);
        std::memcpy(meta.reserved + 0, &v, sizeof(uint32_t));
        return SetMetaInfo(meta);
    }
} // namespace minidb
