// src/storage/page/page_header.h
#pragma once
#include "util/config.h"
#include <cstdint>

namespace minidb {

// 页头结构（放在页的首部）
struct PageHeader {
    uint16_t slot_count;        // 已用槽数
    uint16_t free_space_offset; // 自页首起的可用空间偏移
    uint32_t next_page_id;      // 下一页ID（INVALID_PAGE_ID表示末尾）
    uint32_t page_type;         // 页类型（0=数据页，1=索引页，2=元数据页等）
    uint32_t reserved;          // 保留字段，用于对齐
};

// 槽目录条目（固定4字节）：[offset(2B), length(2B)]
struct SlotEntry { 
    uint16_t offset; 
    uint16_t length; 
};

// 页类型枚举
enum class PageType : uint32_t {
    DATA_PAGE = 0,      // 数据页
    INDEX_PAGE = 1,     // 索引页
    METADATA_PAGE = 2,  // 元数据页
    CATALOG_PAGE = 3    // 目录页
};

// 页内布局常量
constexpr uint16_t PAGE_HEADER_SIZE = sizeof(PageHeader);
constexpr uint16_t SLOT_ENTRY_SIZE = sizeof(SlotEntry);

// 页内布局（以PAGE_SIZE=4096为例）：
// [PageHeader | ...记录区向下增长... | ...空闲... | ...槽目录向上增长... ]
// 记录写入 data + hdr.free_space_offset，槽目录写在页尾（倒序）

} // namespace minidb
