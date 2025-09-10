// src/storage/storage_logger.cpp
#include "storage_logger.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <thread>

namespace minidb {

// 全局logger实例
StorageLogger* g_storage_logger = nullptr;

StorageLogger::StorageLogger(const std::string& filename, LogLevel min_level)
    : min_level_(min_level) {
    // 确保logs目录存在
    std::string full_path;
#ifdef PROJECT_ROOT_DIR
    full_path = std::string(PROJECT_ROOT_DIR) + "/logs/" + filename;
#else
    full_path = filename;
#endif
    
    log_file_.open(full_path, std::ios::app);
    if (!log_file_.is_open()) {
        std::cerr << "Warning: Failed to open log file: " << full_path << std::endl;
    }
}

StorageLogger::~StorageLogger() {
    if (log_file_.is_open()) {
        log_file_.close();
    }
}

void StorageLogger::debug(const std::string& msg) {
    log(LogLevel::DEBUG, "", msg);
}

void StorageLogger::info(const std::string& msg) {
    log(LogLevel::INFO, "", msg);
}

void StorageLogger::warn(const std::string& msg) {
    log(LogLevel::WARN, "", msg);
}

void StorageLogger::error(const std::string& msg) {
    log(LogLevel::ERROR, "", msg);
}

void StorageLogger::debug(const std::string& module, const std::string& msg) {
    log(LogLevel::DEBUG, module, msg);
}

void StorageLogger::info(const std::string& module, const std::string& msg) {
    log(LogLevel::INFO, module, msg);
}

void StorageLogger::warn(const std::string& module, const std::string& msg) {
    log(LogLevel::WARN, module, msg);
}

void StorageLogger::error(const std::string& module, const std::string& msg) {
    log(LogLevel::ERROR, module, msg);
}

void StorageLogger::logPageOperation(const std::string& operation, page_id_t page_id, bool success) {
    std::string status = success ? "SUCCESS" : "FAILED";
    info("PAGE", operation + " page " + std::to_string(page_id) + " - " + status);
    
    // 更新统计
    stats_.total_logs++;
    stats_.info_logs++;
    
    if (operation == "READ") {
        stats_.page_reads++;
    } else if (operation == "WRITE") {
        stats_.page_writes++;
    } else if (operation == "CREATE") {
        stats_.page_creates++;
    } else if (operation == "DELETE") {
        stats_.page_deletes++;
    }
}

void StorageLogger::logCacheHit(bool hit) {
    if (hit) {
        stats_.cache_hits++;
        debug("CACHE", "Cache hit");
    } else {
        stats_.cache_misses++;
        debug("CACHE", "Cache miss");
    }
}

void StorageLogger::logDiskOperation(const std::string& operation, bool success) {
    std::string status = success ? "SUCCESS" : "FAILED";
    info("DISK", operation + " - " + status);
    
    stats_.total_logs++;
    stats_.info_logs++;
    
    if (operation == "READ") {
        stats_.disk_reads++;
    } else if (operation == "WRITE") {
        stats_.disk_writes++;
    }
}

void StorageLogger::logBufferOperation(const std::string& operation, frame_id_t frame_id, bool success) {
    std::string status = success ? "SUCCESS" : "FAILED";
    info("BUFFER", operation + " frame " + std::to_string(frame_id) + " - " + status);
    
    stats_.total_logs++;
    stats_.info_logs++;
    
    if (operation == "PIN") {
        stats_.buffer_pins++;
    } else if (operation == "UNPIN") {
        stats_.buffer_unpins++;
    } else if (operation == "EVICT") {
        stats_.buffer_evictions++;
    }
}

void StorageLogger::printStatistics() {
    std::lock_guard<std::mutex> lock(log_mutex_);
    
    std::cout << "\n=== Storage Logger Statistics ===" << std::endl;
    std::cout << "Total logs: " << stats_.total_logs.load() << std::endl;
    std::cout << "  - DEBUG: " << stats_.debug_logs.load() << std::endl;
    std::cout << "  - INFO:  " << stats_.info_logs.load() << std::endl;
    std::cout << "  - WARN:  " << stats_.warn_logs.load() << std::endl;
    std::cout << "  - ERROR: " << stats_.error_logs.load() << std::endl;
    
    std::cout << "\nPage Operations:" << std::endl;
    std::cout << "  - Reads:   " << stats_.page_reads.load() << std::endl;
    std::cout << "  - Writes:  " << stats_.page_writes.load() << std::endl;
    std::cout << "  - Creates: " << stats_.page_creates.load() << std::endl;
    std::cout << "  - Deletes: " << stats_.page_deletes.load() << std::endl;
    
    std::cout << "\nCache Performance:" << std::endl;
    uint64_t total_cache_access = stats_.cache_hits.load() + stats_.cache_misses.load();
    if (total_cache_access > 0) {
        double hit_rate = (double)stats_.cache_hits.load() / total_cache_access * 100.0;
        std::cout << "  - Hits:    " << stats_.cache_hits.load() << std::endl;
        std::cout << "  - Misses:  " << stats_.cache_misses.load() << std::endl;
        std::cout << "  - Hit Rate: " << std::fixed << std::setprecision(2) << hit_rate << "%" << std::endl;
    } else {
        std::cout << "  - No cache operations recorded" << std::endl;
    }
    
    std::cout << "\nDisk Operations:" << std::endl;
    std::cout << "  - Reads:  " << stats_.disk_reads.load() << std::endl;
    std::cout << "  - Writes: " << stats_.disk_writes.load() << std::endl;
    
    std::cout << "\nBuffer Operations:" << std::endl;
    std::cout << "  - Pins:     " << stats_.buffer_pins.load() << std::endl;
    std::cout << "  - Unpins:   " << stats_.buffer_unpins.load() << std::endl;
    std::cout << "  - Evictions:" << stats_.buffer_evictions.load() << std::endl;
    std::cout << "================================\n" << std::endl;
}

void StorageLogger::resetStatistics() {
    stats_.total_logs = 0;
    stats_.debug_logs = 0;
    stats_.info_logs = 0;
    stats_.warn_logs = 0;
    stats_.error_logs = 0;
    
    stats_.page_reads = 0;
    stats_.page_writes = 0;
    stats_.page_creates = 0;
    stats_.page_deletes = 0;
    
    stats_.cache_hits = 0;
    stats_.cache_misses = 0;
    
    stats_.disk_reads = 0;
    stats_.disk_writes = 0;
    
    stats_.buffer_pins = 0;
    stats_.buffer_unpins = 0;
    stats_.buffer_evictions = 0;
}

void StorageLogger::log(LogLevel level, const std::string& module, const std::string& msg) {
    if (level < min_level_) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(log_mutex_);
    
    // 更新统计
    stats_.total_logs++;
    switch (level) {
        case LogLevel::DEBUG: stats_.debug_logs++; break;
        case LogLevel::INFO:  stats_.info_logs++; break;
        case LogLevel::WARN:  stats_.warn_logs++; break;
        case LogLevel::ERROR: stats_.error_logs++; break;
    }
    
    // 写入日志
    if (log_file_.is_open()) {
        log_file_ << getLogPrefix(level, module) << msg << std::endl;
        log_file_.flush();
    }
    
    // 同时输出到控制台（ERROR和WARN级别）
    if (level >= LogLevel::WARN) {
        std::cout << getLogPrefix(level, module) << msg << std::endl;
    }
}

std::string StorageLogger::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

std::string StorageLogger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO ";
        case LogLevel::WARN:  return "WARN ";
        case LogLevel::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

std::string StorageLogger::getLogPrefix(LogLevel level, const std::string& module) {
    std::stringstream ss;
    ss << "[" << getCurrentTimestamp() << "] "
       << "[" << levelToString(level) << "] ";
    
    if (!module.empty()) {
        ss << "[" << module << "] ";
    }
    
    ss << "[T" << std::this_thread::get_id() << "] ";
    return ss.str();
}

// 全局函数实现
void initStorageLogger(const std::string& filename, LogLevel level) {
    if (g_storage_logger == nullptr) {
        g_storage_logger = new StorageLogger(filename, level);
    }
}

void cleanupStorageLogger() {
    if (g_storage_logger != nullptr) {
        delete g_storage_logger;
        g_storage_logger = nullptr;
    }
}

} // namespace minidb
