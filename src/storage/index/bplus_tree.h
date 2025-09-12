#pragma once
#include "storage/storage_engine.h"
#include <cstdint>
#include <vector>
#include <optional>

namespace minidb {

struct RID {
    page_id_t page_id;
    uint16_t slot;
};

class BPlusTree {
public:
    explicit BPlusTree(StorageEngine* engine);

    // 创建一个新的 B+ 树（单叶子根），返回根页号
    page_id_t CreateNew();

    // 设置已有根（例如从 Catalog 读取）
    void SetRoot(page_id_t root_id);
    page_id_t GetRoot() const { return root_page_id_; }

    // 基本操作（最小版：支持叶子分裂与根提升；不支持多层级级联分裂）
    bool Insert(int32_t key, const RID& rid);
    std::optional<RID> Search(int32_t key);
    std::vector<RID> Range(int32_t low, int32_t high);

private:
    // 节点页内布局（存放在 Page 的数据区内，紧随 PageHeader 之后）
    struct NodeHeader {
        uint8_t is_leaf;           // 1=leaf, 0=internal（当前仅实现叶子）
        uint8_t reserved8[3];
        uint16_t key_count;        // 当前键数量
        uint16_t reserved16;
        page_id_t parent;          // 父节点（未用）
        page_id_t next;            // 叶子横向链（范围扫描）
        page_id_t prev;            // 叶子横向链
    };

    struct LeafEntry {
        int32_t key;
        page_id_t rid_page;
        uint16_t rid_slot;
        uint16_t pad;
    };

    static constexpr size_t NodeHeaderSize = sizeof(NodeHeader);
    static constexpr size_t LeafEntrySize = sizeof(LeafEntry);
    struct InternalArrays {
        int32_t* keys;             // 大小 = key_count
        page_id_t* children;       // 大小 = key_count + 1
    };

    // 访问 Page 内节点头与条目区
    static NodeHeader* GetNodeHeader(Page* page);
    static const NodeHeader* GetNodeHeaderConst(const Page* page);
    static LeafEntry* GetLeafEntries(Page* page);
    static const LeafEntry* GetLeafEntriesConst(const Page* page);
    static uint16_t GetLeafMaxEntries();
    // 内节点数组视图与容量
    static InternalArrays GetInternalArrays(Page* page);
    static const InternalArrays GetInternalArraysConst(const Page* page);
    static uint16_t GetInternalMaxKeys();

    static void InitializeLeaf(Page* page);
    static void InitializeInternal(Page* page);

    // 查找叶子（从根下降一层，当前仅支持高度<=2）
    Page* DescendToLeaf(int32_t key);
    // 叶子插入（有序插入，若溢出则分裂并根提升）
    bool InsertIntoLeafAndSplitIfNeeded(Page* leaf, int32_t key, const RID& rid);
    // 根从单叶提升为内节点，挂接两个叶子
    void PromoteNewRoot(Page* left_leaf, Page* right_leaf, int32_t separator_key);

    StorageEngine* engine_;
    page_id_t root_page_id_{INVALID_PAGE_ID};
};

} // namespace minidb


