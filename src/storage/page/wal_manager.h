#pragma once
#include "util/config.h"
#include <string>
#include <fstream>
#include <mutex>

namespace minidb {

class DiskManager; // forward

class WalManager {
public:
    explicit WalManager(const std::string& wal_file) : wal_file_(wal_file) {}

    // 物理页级别 WAL：写入 (page_id, PAGE_SIZE bytes)
    bool Append(page_id_t page_id, const char* page_data);

    // 恢复：顺序重放 WAL 到数据文件
    bool Recover(DiskManager& dm);

    // 截断 WAL（checkpoint 之后）
    bool Truncate();

private:
    std::string wal_file_;
    std::mutex mtx_;
};

}


