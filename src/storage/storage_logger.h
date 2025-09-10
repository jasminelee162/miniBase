// src/storage/storage_logger.h
#pragma once
#include <string>
#include <fstream>
#include <chrono>
#include <mutex>
#include <atomic>
#include <map>
#include "util/config.h"  // 包含正确的类型定义

namespace minidb {

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3
};

class StorageLogger {
public:
    StorageLogger(const std::string& filename, LogLevel min_level = LogLevel::INFO);
    ~StorageLogger();

    // 基本日志方法
    void debug(const std::string& msg);
    void info(const std::string& msg);
    void warn(const std::string& msg);
    void error(const std::string& msg);

    // 带模块标识的日志方法
    void debug(const std::string& module, const std::string& msg);
    void info(const std::string& module, const std::string& msg);
    void warn(const std::string& module, const std::string& msg);
    void error(const std::string& module, const std::string& msg);

    // 统计相关方法
    void logPageOperation(const std::string& operation, page_id_t page_id, bool success = true);
    void logCacheHit(bool hit);
    void logDiskOperation(const std::string& operation, bool success = true);
    void logBufferOperation(const std::string& operation, frame_id_t frame_id, bool success = true);

    // 获取统计信息
    struct Statistics {
        std::atomic<uint64_t> total_logs{0};
        std::atomic<uint64_t> debug_logs{0};
        std::atomic<uint64_t> info_logs{0};
        std::atomic<uint64_t> warn_logs{0};
        std::atomic<uint64_t> error_logs{0};
        
        std::atomic<uint64_t> page_reads{0};
        std::atomic<uint64_t> page_writes{0};
        std::atomic<uint64_t> page_creates{0};
        std::atomic<uint64_t> page_deletes{0};
        
        std::atomic<uint64_t> cache_hits{0};
        std::atomic<uint64_t> cache_misses{0};
        
        std::atomic<uint64_t> disk_reads{0};
        std::atomic<uint64_t> disk_writes{0};
        
        std::atomic<uint64_t> buffer_pins{0};
        std::atomic<uint64_t> buffer_unpins{0};
        std::atomic<uint64_t> buffer_evictions{0};
    };

    const Statistics& getStatistics() const { return stats_; }
    void printStatistics();
    void resetStatistics();

    // 设置日志级别
    void setLogLevel(LogLevel level) { min_level_ = level; }
    LogLevel getLogLevel() const { return min_level_; }

private:
    void log(LogLevel level, const std::string& module, const std::string& msg);
    std::string getCurrentTimestamp();
    std::string levelToString(LogLevel level);
    std::string getLogPrefix(LogLevel level, const std::string& module);

    std::ofstream log_file_;
    LogLevel min_level_;
    std::mutex log_mutex_;
    Statistics stats_;
};

// 全局storage logger实例
extern StorageLogger* g_storage_logger;

// 初始化全局logger
void initStorageLogger(const std::string& filename = "storage.log", LogLevel level = LogLevel::INFO);

// 清理全局logger
void cleanupStorageLogger();

} // namespace minidb
