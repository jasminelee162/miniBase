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
    // 启动时尝试从存储加载已保存的根
    void LoadRootFromStorage();

    // 基本操作（最小版：支持叶子分裂与根提升；不支持多层级级联分裂）
    bool Insert(int32_t key, const RID& rid);
    std::optional<RID> Search(int32_t key);
    std::vector<RID> Range(int32_t low, int32_t high);

    
    // 增强操作
    bool Delete(int32_t key);
    bool Update(int32_t key, const RID& new_rid);
    bool HasKey(int32_t key);
    size_t GetKeyCount() const;
    
    // 模板化操作（支持多种键类型）
    template<typename KeyType>
    bool InsertGeneric(const KeyType& key, const RID& rid);
    
    template<typename KeyType>
    std::optional<RID> SearchGeneric(const KeyType& key);
    
    template<typename KeyType>
    bool DeleteGeneric(const KeyType& key);


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
    static uint16_t GetLeafMinEntries() { return static_cast<uint16_t>(GetLeafMaxEntries() / 2); }
    // 内节点数组视图与容量
    static InternalArrays GetInternalArrays(Page* page);
    static const InternalArrays GetInternalArraysConst(const Page* page);
    static uint16_t GetInternalMaxKeys();
    static uint16_t GetInternalMinKeys() { return static_cast<uint16_t>(GetInternalMaxKeys() / 2); }

    static void InitializeLeaf(Page* page);
    static void InitializeInternal(Page* page);

    // 查找叶子（从根下降一层，当前仅支持高度<=2）
    Page* DescendToLeaf(int32_t key);
    // 叶子插入（有序插入，若溢出则分裂并根提升）
    bool InsertIntoLeafAndSplitIfNeeded(Page* leaf, int32_t key, const RID& rid);
    // 根从单叶提升为内节点，挂接两个叶子
    void PromoteNewRoot(Page* left_leaf, Page* right_leaf, int32_t separator_key);

    // 向父节点插入分隔键（leaf/internal 分裂后使用），若父也满则递归向上分裂
    void InsertIntoParent(page_id_t left_child, page_id_t right_child, int32_t separator_key);
    // 在内节点中插入 <key, right_child> 到指定位置（key 放在 children[left_index] 与 children[left_index+1] 之间）
    bool InsertIntoInternal(Page* parent, int insert_pos, int32_t key, page_id_t right_child);
    // 如果内节点已满，执行分裂并将分隔键向上递归
    void SplitInternalAndPropagate(Page* parent, int32_t key, page_id_t right_child);
    // 寻找父节点中 left_child 的位置
    int FindChildIndex(Page* parent, page_id_t left_child_id);

    
    // 删除相关辅助方法
    bool DeleteFromLeaf(Page* leaf, int32_t key);
    bool UpdateInLeaf(Page* leaf, int32_t key, const RID& new_rid);
    int32_t FindKeyIndex(const LeafEntry* entries, uint16_t count, int32_t key);

    // 删除重平衡（叶子与内节点）
    bool DeleteAndRebalance(int32_t key);
    void RebalanceLeaf(Page* leaf);
    void RebalanceInternal(page_id_t node_id);
    int32_t GetLeafFirstKey(const Page* leaf) const;
    void RemoveParentKey(page_id_t parent_id, int key_index);
    void ReplaceParentKey(page_id_t parent_id, int key_index, int32_t new_key);
    
    // 键类型转换辅助方法
    template<typename KeyType>
    int32_t ConvertToInt32(const KeyType& key);
    
    template<typename KeyType>
    KeyType ConvertFromInt32(int32_t value);


    StorageEngine* engine_;
    page_id_t root_page_id_{INVALID_PAGE_ID};
};

} // namespace minidb


