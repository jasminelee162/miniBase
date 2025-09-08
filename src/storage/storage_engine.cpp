#include "storage/storage_engine.h"
#include <iostream>

namespace minidb {

StorageEngine::StorageEngine(const std::string& db_file, size_t buffer_pool_size)
    : disk_manager_(std::make_unique<DiskManager>(db_file)),
      buffer_pool_manager_(std::make_unique<BufferPoolManager>(buffer_pool_size, disk_manager_.get())),
      db_file_(db_file) {}

StorageEngine::~StorageEngine() {
    Shutdown();
}

Page* StorageEngine::GetPage(page_id_t page_id) {
    return buffer_pool_manager_->FetchPage(page_id);
}

Page* StorageEngine::CreatePage(page_id_t* page_id) {
    return buffer_pool_manager_->NewPage(page_id);
}

bool StorageEngine::PutPage(page_id_t page_id, bool is_dirty) {
    return buffer_pool_manager_->UnpinPage(page_id, is_dirty);
}

bool StorageEngine::RemovePage(page_id_t page_id) {
    return buffer_pool_manager_->DeletePage(page_id);
}

std::vector<Page*> StorageEngine::GetPages(const std::vector<page_id_t>& page_ids) {
    std::vector<Page*> result;
    result.reserve(page_ids.size());
    for (auto pid : page_ids) {
        result.push_back(buffer_pool_manager_->FetchPage(pid));
    }
    return result;
}

void StorageEngine::Shutdown() {
    if (is_shutdown_.exchange(true)) return;
    if (buffer_pool_manager_) buffer_pool_manager_->FlushAllPages();
    if (disk_manager_) disk_manager_->Shutdown();
}

void StorageEngine::Checkpoint() {
    if (buffer_pool_manager_) buffer_pool_manager_->FlushAllPages();
}

void StorageEngine::PrintStats() const {
    std::cout << "BufferPoolSize=" << GetBufferPoolSize()
              << ", HitRate=" << GetCacheHitRate() << std::endl;
}

double StorageEngine::GetCacheHitRate() const {
    return buffer_pool_manager_ ? buffer_pool_manager_->GetHitRate() : 0.0;
}

size_t StorageEngine::GetBufferPoolSize() const {
    return buffer_pool_manager_ ? buffer_pool_manager_->GetPoolSize() : 0;
}

bool StorageEngine::AdjustBufferPoolSize(size_t new_size) {
    return buffer_pool_manager_ ? buffer_pool_manager_->ResizePool(new_size) : false;
}

} // namespace minidb

