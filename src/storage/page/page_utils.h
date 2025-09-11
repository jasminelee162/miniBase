// src/storage/page/page_utils.h
#pragma once
#include "page.h"
#include "page_header.h"
#include <functional>
#include <cstring>

namespace minidb {

// 获取槽目录条目
inline SlotEntry* GetSlot(Page* page, uint16_t slot_index) {
    char* base = page->GetData();
    // 槽 i 位于：页尾起始位置 - (i+1)*sizeof(SlotEntry)
    size_t slot_bytes = SLOT_ENTRY_SIZE * (static_cast<size_t>(slot_index) + 1);
    return reinterpret_cast<SlotEntry*>(base + PAGE_SIZE - slot_bytes);
}

inline const SlotEntry* GetSlot(const Page* page, uint16_t slot_index) {
    const char* base = page->GetData();
    size_t slot_bytes = SLOT_ENTRY_SIZE * (static_cast<size_t>(slot_index) + 1);
    return reinterpret_cast<const SlotEntry*>(base + PAGE_SIZE - slot_bytes);
}

// 计算当前剩余可用空间（不含槽目录本身）
inline uint16_t GetFreeSpace(const Page* page) {
    const PageHeader* hdr = page->GetHeader();
    uint16_t used_for_slots = hdr->slot_count * SLOT_ENTRY_SIZE;
    // 槽目录占据页尾的 used_for_slots 字节
    return PAGE_SIZE - used_for_slots - hdr->free_space_offset;
}

// 追加一条记录；成功返回 true，并在 out_slot 返回槽序号
inline bool AppendRow(Page* page, const void* row, uint16_t len, uint16_t* out_slot = nullptr) {
    PageHeader* hdr = page->GetHeader();
    
    // 需要的空间：记录 len + 新增一个槽条目
    uint16_t need = len + SLOT_ENTRY_SIZE;
    if (GetFreeSpace(page) < need) return false;
    
    // 写入记录
    uint16_t write_off = hdr->free_space_offset;
    std::memcpy(page->GetData() + write_off, row, len);
    hdr->free_space_offset = write_off + len;
    
    // 写入槽目录（页尾）
    SlotEntry* slot = GetSlot(page, hdr->slot_count);
    slot->offset = write_off;
    slot->length = len;
    if (out_slot) *out_slot = hdr->slot_count;
    hdr->slot_count++;
    // Mark page dirty after in-memory mutation
    page->SetDirty(true);
    return true;
}

// 遍历所有记录（回调接收每条记录的起始指针与长度）
inline void ForEachRow(Page* page, const std::function<void(const unsigned char*, uint16_t)>& fn) {
    const unsigned char* data = reinterpret_cast<const unsigned char*>(page->GetData());
    const PageHeader* hdr = page->GetHeader();
    for (uint16_t i = 0; i < hdr->slot_count; ++i) {
        const SlotEntry* s = GetSlot(page, i);
        fn(data + s->offset, s->length);
    }
}

// 获取指定槽的记录
inline const unsigned char* GetRow(const Page* page, uint16_t slot_index, uint16_t* out_length = nullptr) {
    if (slot_index >= page->GetSlotCount()) return nullptr;
    
    const SlotEntry* slot = GetSlot(page, slot_index);
    if (out_length) *out_length = slot->length;
    return reinterpret_cast<const unsigned char*>(page->GetData()) + slot->offset;
}

// 删除指定槽的记录（标记为删除，不实际移动数据）
inline bool DeleteRow(Page* page, uint16_t slot_index) {
    if (slot_index >= page->GetSlotCount()) return false;
    
    SlotEntry* slot = GetSlot(page, slot_index);
    slot->length = 0;  // 标记为删除
    return true;
}

// 页链接操作
inline page_id_t GetNextPageId(const Page* page) {
    return page->GetNextPageId();
}

inline void SetNextPageId(Page* page, page_id_t next_id) {
    page->SetNextPageId(next_id);
}

// 检查页是否为空
inline bool IsPageEmpty(const Page* page) {
    return page->GetSlotCount() == 0;
}

// 检查页是否有足够空间
inline bool HasSpaceFor(Page* page, uint16_t record_size) {
    return GetFreeSpace(page) >= (record_size + SLOT_ENTRY_SIZE);
}

} // namespace minidb
