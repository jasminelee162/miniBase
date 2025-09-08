#include "storage/page/disk_manager.h"
#include <cassert>
#include <cstring>
#include <future>

namespace minidb {

DiskManager::DiskManager(const std::string& db_file) : db_file_(db_file) {
    std::lock_guard<std::mutex> lock(file_mutex_);
    file_stream_.open(db_file_, std::ios::in | std::ios::out | std::ios::binary);
    if (!file_stream_.is_open()) {
        // Create if not exists
        std::fstream create_stream(db_file_, std::ios::out | std::ios::binary);
        create_stream.close();
        file_stream_.open(db_file_, std::ios::in | std::ios::out | std::ios::binary);
    }
    file_stream_.seekg(0, std::ios::end);
    std::streamoff size = file_stream_.tellg();
    if (size > 0) {
        // Infer next_page_id_ from file size
        next_page_id_.store(static_cast<page_id_t>(size / PAGE_SIZE + 1));
    }
}

DiskManager::~DiskManager() {
    Shutdown();
}

Status DiskManager::ReadPage(page_id_t page_id, char* page_data) {
    if (page_id == INVALID_PAGE_ID || page_data == nullptr) {
        return Status::INVALID_PARAM;
    }
    std::lock_guard<std::mutex> lock(file_mutex_);
    if (is_shutdown_.load()) {
        return Status::IO_ERROR;
    }
    size_t offset = GetFileOffset(page_id);
    file_stream_.seekg(0, std::ios::end);
    std::streamoff file_size = file_stream_.tellg();
    if (static_cast<std::streamoff>(offset) >= file_size) {
        // Reading beyond EOF: return zero-filled page
        std::memset(page_data, 0, PAGE_SIZE);
        return Status::OK;
    }
    file_stream_.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
    file_stream_.read(page_data, PAGE_SIZE);
    if (file_stream_.gcount() < static_cast<std::streamsize>(PAGE_SIZE)) {
        // Short read, zero remainder
        std::streamsize read_cnt = file_stream_.gcount();
        std::memset(page_data + read_cnt, 0, PAGE_SIZE - static_cast<size_t>(read_cnt));
    }
    if (!file_stream_) {
        return Status::IO_ERROR;
    }
    num_reads_.fetch_add(1);
    if constexpr (ENABLE_STORAGE_LOG) {
        fprintf(stderr, "[DM] Read page %u\n", (unsigned)page_id);
    }
    return Status::OK;
}

Status DiskManager::WritePage(page_id_t page_id, const char* page_data) {
    if (page_id == INVALID_PAGE_ID || page_data == nullptr) {
        return Status::INVALID_PARAM;
    }
    std::lock_guard<std::mutex> lock(file_mutex_);
    if (is_shutdown_.load()) {
        return Status::IO_ERROR;
    }
    size_t offset = GetFileOffset(page_id);
    file_stream_.seekp(static_cast<std::streamoff>(offset), std::ios::beg);
    file_stream_.write(page_data, PAGE_SIZE);
    file_stream_.flush();
    if (!file_stream_) {
        return Status::IO_ERROR;
    }
    num_writes_.fetch_add(1);
    if constexpr (ENABLE_STORAGE_LOG) {
        fprintf(stderr, "[DM] Write page %u\n", (unsigned)page_id);
    }
    // Advance next_page_id_ if we wrote past previous end
    page_id_t expected_next = static_cast<page_id_t>(offset / PAGE_SIZE + 2);
    if (expected_next > next_page_id_.load()) {
        next_page_id_.store(expected_next);
    }
    return Status::OK;
}

std::future<Status> DiskManager::ReadPageAsync(page_id_t page_id, char* page_data) {
    return std::async(std::launch::async, [this, page_id, page_data]() { return ReadPage(page_id, page_data); });
}

std::future<Status> DiskManager::WritePageAsync(page_id_t page_id, const char* page_data) {
    return std::async(std::launch::async, [this, page_id, page_data]() { return WritePage(page_id, page_data); });
}

page_id_t DiskManager::AllocatePage() {
    std::lock_guard<std::mutex> lock(file_mutex_);
    // 先复用空闲页
    if (!free_page_ids_.empty()) {
        page_id_t pid = free_page_ids_.front();
        free_page_ids_.pop();
        return pid;
    }
    return next_page_id_.fetch_add(1);
}

void DiskManager::DeallocatePage(page_id_t page_id) {
    if (page_id == INVALID_PAGE_ID) return;
    std::lock_guard<std::mutex> lock(file_mutex_);
    // 简单复用：加入空闲队列（不做去重，调用方需保证合法性）
    free_page_ids_.push(page_id);
}

void DiskManager::FlushAllPages() {
    std::lock_guard<std::mutex> lock(file_mutex_);
    file_stream_.flush();
}

void DiskManager::Shutdown() {
    std::lock_guard<std::mutex> lock(file_mutex_);
    if (is_shutdown_.exchange(true)) {
        return;
    }
    if (file_stream_.is_open()) {
        file_stream_.flush();
        file_stream_.close();
    }
}

} // namespace minidb

