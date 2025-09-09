#include "storage/buffer/buffer_pool_manager.h"
#include "util/logger.h"
#include <cassert>

namespace minidb {

#ifdef PROJECT_ROOT_DIR
static Logger g_storage_logger_bpm(std::string(PROJECT_ROOT_DIR) + "/logs/storage.log");
#else
static Logger g_storage_logger_bpm("storage.log");
#endif

BufferPoolManager::BufferPoolManager(size_t pool_size, DiskManager* disk_manager)
    : pool_size_(pool_size), disk_manager_(disk_manager) {
    pages_ = new Page[pool_size_];
    lru_replacer_ = std::make_unique<LRUReplacer>(pool_size_);
    fifo_replacer_ = std::make_unique<FIFOReplacer>(pool_size_);
    frame_page_ids_.assign(pool_size_, INVALID_PAGE_ID);
    for (frame_id_t i = 0; i < pool_size_; ++i) {
        free_list_.push_back(i);
    }
}

BufferPoolManager::~BufferPoolManager() {
    FlushAllPages();
    delete[] pages_;
}

frame_id_t BufferPoolManager::FindVictimFrame() {
    // 先尝试空闲帧
    {
        std::lock_guard<std::mutex> guard(free_list_mutex_);
        if (!free_list_.empty()) {
            frame_id_t fid = free_list_.front();
            free_list_.pop_front();
            return fid;
        }
    }
    // 其次从替换器获取
    frame_id_t victim = INVALID_FRAME_ID;
    if ((policy_ == ReplacementPolicy::LRU ? lru_replacer_->Victim(&victim)
                                           : fifo_replacer_->Victim(&victim))) {
        num_replacements_.fetch_add(1);
        return victim;
    }
    return INVALID_FRAME_ID;
}

bool BufferPoolManager::FlushFrameToPages(frame_id_t frame_id) {
    Page& page = pages_[frame_id];
    if (frame_page_ids_[frame_id] == INVALID_PAGE_ID) return true;
    if (!page.IsDirty()) return true;
    Status s = disk_manager_->WritePage(frame_page_ids_[frame_id], page.GetData());
    if (s != Status::OK) return false;
    page.SetDirty(false);
    num_writebacks_.fetch_add(1);
    if constexpr (ENABLE_STORAGE_LOG) {
        g_storage_logger_bpm.log(
            std::string("[BPM] Writeback page ") +
            std::to_string((unsigned)frame_page_ids_[frame_id]) +
            " from frame " + std::to_string((size_t)frame_id)
        );
    }
    return true;
}
//从缓存池里获取一页
Page* BufferPoolManager::FetchPage(page_id_t page_id) {
    std::unique_lock<std::shared_mutex> lock(latch_);
    num_accesses_.fetch_add(1);
    auto it = page_table_.find(page_id);
    if (it != page_table_.end()) {
        frame_id_t fid = it->second;
        Page& page = pages_[fid];
        page.IncPinCount();
        if (policy_ == ReplacementPolicy::LRU) lru_replacer_->Pin(fid); else fifo_replacer_->Pin(fid);
        num_hits_.fetch_add(1);
        return &page;
    }

    frame_id_t fid = FindVictimFrame();
    if (fid == INVALID_FRAME_ID) {
        return nullptr;
    }
    // 若需要，落盘旧页
    Page& frame_page = pages_[fid];
    if (frame_page_ids_[fid] != INVALID_PAGE_ID) {
        if (!FlushFrameToPages(fid)) {
            // 失败则回退
            return nullptr;
        }
        page_table_.erase(frame_page_ids_[fid]);
        frame_page.Reset();
        frame_page_ids_[fid] = INVALID_PAGE_ID;
    }

    // 从磁盘读入目标页
    Status s = disk_manager_->ReadPage(page_id, frame_page.GetData());
    if (s != Status::OK) {
        // 读失败，回收该帧到空闲列表
        std::lock_guard<std::mutex> guard(free_list_mutex_);
        free_list_.push_front(fid);
        return nullptr;
    }
    frame_page.SetDirty(false);
    page_table_[page_id] = fid;
    frame_page_ids_[fid] = page_id;
    // 初始化元信息
    // Note: Page::Reset 会清空数据，这里不调用。仅设置 pin
    // 我们不在 Page 中持久保存 page_id_ 的 setter，因此不依赖内部 page_id_ 字段用于磁盘定位。
    frame_page.IncPinCount();
    if (policy_ == ReplacementPolicy::LRU) lru_replacer_->Pin(fid); else fifo_replacer_->Pin(fid);
    return &frame_page;
}
//申请新页 向DiskManager申请新页号,找一个槽位，清空页内容，pin 并返回
Page* BufferPoolManager::NewPage(page_id_t* page_id) {
    if (page_id == nullptr) return nullptr;
    std::unique_lock<std::shared_mutex> lock(latch_);
    frame_id_t fid = FindVictimFrame();
    if (fid == INVALID_FRAME_ID) {
        return nullptr;
    }
    // 淘汰旧页
    Page& frame_page = pages_[fid];
    if (frame_page_ids_[fid] != INVALID_PAGE_ID) {
        if (!FlushFrameToPages(fid)) {
            return nullptr;
        }
        page_table_.erase(frame_page_ids_[fid]);
        frame_page.Reset();
        frame_page_ids_[fid] = INVALID_PAGE_ID;
    }

    // 分配新页并清零（磁盘满时返回 INVALID_PAGE_ID）
    *page_id = disk_manager_->AllocatePage();
    if (*page_id == INVALID_PAGE_ID) {
        // 回收该帧到空闲列表并返回失败
        page_table_.erase(frame_page_ids_[fid]);
        frame_page.Reset();
        frame_page_ids_[fid] = INVALID_PAGE_ID;
        std::lock_guard<std::mutex> guard(free_list_mutex_);
        free_list_.push_front(fid);
        return nullptr;
    }
    std::memset(frame_page.GetData(), 0, PAGE_SIZE);
    page_table_[*page_id] = fid;
    frame_page_ids_[fid] = *page_id;
    frame_page.SetDirty(false);
    frame_page.IncPinCount();
    if (policy_ == ReplacementPolicy::LRU) lru_replacer_->Pin(fid); else fifo_replacer_->Pin(fid);
    return &frame_page;
}
//进程用完归还缓存，标记脏否
bool BufferPoolManager::UnpinPage(page_id_t page_id, bool is_dirty) {
    std::unique_lock<std::shared_mutex> lock(latch_);
    auto it = page_table_.find(page_id);
    if (it == page_table_.end()) return false;
    frame_id_t fid = it->second;
    Page& page = pages_[fid];
    if (is_dirty) page.SetDirty(true);
    if (page.GetPinCount() <= 0) return false;
    page.DecPinCount();
    if (page.GetPinCount() == 0) {
        if (policy_ == ReplacementPolicy::LRU) lru_replacer_->Unpin(fid); else fifo_replacer_->Unpin(fid);
    }
    return true;
}
//把该页写回磁盘并清除脏标记
bool BufferPoolManager::FlushPage(page_id_t page_id) {
    std::shared_lock<std::shared_mutex> lock(latch_);
    auto it = page_table_.find(page_id);
    if (it == page_table_.end()) return false;
    frame_id_t fid = it->second;
    Page& page = pages_[fid];
    if (frame_page_ids_[fid] == INVALID_PAGE_ID) return false;
    Status s = disk_manager_->WritePage(page_id, page.GetData());
    if (s != Status::OK) return false;
    page.SetDirty(false);
    return true;
}
//只有当页未被使用（pin=0）时才删；若脏则先写回
bool BufferPoolManager::DeletePage(page_id_t page_id) {
    std::unique_lock<std::shared_mutex> lock(latch_);
    auto it = page_table_.find(page_id);
    if (it != page_table_.end()) {
        frame_id_t fid = it->second;
        Page& page = pages_[fid];
        if (page.GetPinCount() > 0) return false; // 仍被引用
        // 脏页落盘（可选：若是删除可跳过写回，这里简单处理）
        if (page.IsDirty()) {
            if (disk_manager_->WritePage(page_id, page.GetData()) != Status::OK) {
                return false;
            }
        }
        page_table_.erase(it);
        page.Reset();
        frame_page_ids_[fid] = INVALID_PAGE_ID;
        std::lock_guard<std::mutex> guard(free_list_mutex_);
        free_list_.push_front(fid);
    }
    disk_manager_->DeallocatePage(page_id);
    return true;
}
//把所有脏页写回，并调用 disk_manager_->FlushAllPages()
void BufferPoolManager::FlushAllPages() {
    std::shared_lock<std::shared_mutex> lock(latch_);
    for (auto& kv : page_table_) {
        frame_id_t fid = kv.second;
        Page& page = pages_[fid];
        if (frame_page_ids_[fid] == INVALID_PAGE_ID) continue;
        if (page.IsDirty()) {
            disk_manager_->WritePage(kv.first, page.GetData());
            page.SetDirty(false);
        }
    }
    disk_manager_->FlushAllPages();
}

double BufferPoolManager::GetHitRate() const {
    size_t accesses = num_accesses_.load();
    if (accesses == 0) return 0.0;
    return static_cast<double>(num_hits_.load()) / static_cast<double>(accesses);
}

size_t BufferPoolManager::GetFreeFramesCount() const {
    std::lock_guard<std::mutex> guard(const_cast<std::mutex&>(free_list_mutex_));
    return free_list_.size();
}

bool BufferPoolManager::ResizePool(size_t /*new_size*/) {
    // 非必要特性：暂不实现动态调整
    return false;
}

} // namespace minidb

