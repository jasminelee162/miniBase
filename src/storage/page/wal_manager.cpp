#include "storage/page/wal_manager.h"
#include "storage/page/disk_manager.h"
#include <vector>

namespace minidb {

static constexpr uint64_t WAL_MAGIC = 0x4D444257414C5F31ULL; // MDBWAL_1

struct WalRecordHeader {
    uint64_t magic;
    uint32_t page_id;
    uint32_t length; // always PAGE_SIZE
};

bool WalManager::Append(page_id_t page_id, const char* page_data) {
    std::lock_guard<std::mutex> g(mtx_);
    std::ofstream ofs(wal_file_, std::ios::binary | std::ios::app);
    if (!ofs.is_open()) return false;
    WalRecordHeader hdr{WAL_MAGIC, static_cast<uint32_t>(page_id), static_cast<uint32_t>(PAGE_SIZE)};
    ofs.write(reinterpret_cast<const char*>(&hdr), sizeof(hdr));
    ofs.write(page_data, PAGE_SIZE);
    ofs.flush();
    return static_cast<bool>(ofs);
}

bool WalManager::Recover(DiskManager& dm) {
    std::lock_guard<std::mutex> g(mtx_);
    std::ifstream ifs(wal_file_, std::ios::binary);
    if (!ifs.is_open()) return true; // no wal, ok
    while (true) {
        WalRecordHeader hdr{};
        ifs.read(reinterpret_cast<char*>(&hdr), sizeof(hdr));
        if (!ifs) break;
        if (hdr.magic != WAL_MAGIC || hdr.length != PAGE_SIZE) {
            break;
        }
        std::vector<char> buf(PAGE_SIZE);
        ifs.read(buf.data(), PAGE_SIZE);
        if (!ifs) break;
        dm.WritePage(static_cast<page_id_t>(hdr.page_id), buf.data());
    }
    return true;
}

bool WalManager::Truncate() {
    std::lock_guard<std::mutex> g(mtx_);
    std::ofstream ofs(wal_file_, std::ios::binary | std::ios::trunc);
    return static_cast<bool>(ofs);
}

}


