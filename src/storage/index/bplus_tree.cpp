#include "storage/index/bplus_tree.h"
#include "storage/page/page_header.h"
#include <cstring>
#include <algorithm>
//B+树的实现
namespace minidb {

static inline char* PagePayload(Page* page) {
    return page->GetData() + PAGE_HEADER_SIZE;
}

BPlusTree::BPlusTree(StorageEngine* engine) : engine_(engine) {}

page_id_t BPlusTree::CreateNew() {
    page_id_t pid = INVALID_PAGE_ID;
    Page* p = engine_->CreatePage(&pid);
    if (!p) return INVALID_PAGE_ID;
    p->InitializePage(PageType::INDEX_PAGE);
    InitializeLeaf(p);
    engine_->PutPage(pid, true);
    root_page_id_ = pid;
    return pid;
}

void BPlusTree::SetRoot(page_id_t root_id) {
    root_page_id_ = root_id;
}

BPlusTree::NodeHeader* BPlusTree::GetNodeHeader(Page* page) {
    return reinterpret_cast<NodeHeader*>(PagePayload(page));
}

const BPlusTree::NodeHeader* BPlusTree::GetNodeHeaderConst(const Page* page) {
    return reinterpret_cast<const NodeHeader*>(page->GetData() + PAGE_HEADER_SIZE);
}

BPlusTree::LeafEntry* BPlusTree::GetLeafEntries(Page* page) {
    return reinterpret_cast<LeafEntry*>(PagePayload(page) + NodeHeaderSize);
}

const BPlusTree::LeafEntry* BPlusTree::GetLeafEntriesConst(const Page* page) {
    return reinterpret_cast<const LeafEntry*>(page->GetData() + PAGE_HEADER_SIZE + NodeHeaderSize);
}

uint16_t BPlusTree::GetLeafMaxEntries() {
    size_t payload = PAGE_SIZE - PAGE_HEADER_SIZE;
    if (payload < NodeHeaderSize) return 0;
    return static_cast<uint16_t>((payload - NodeHeaderSize) / LeafEntrySize);
}

void BPlusTree::InitializeLeaf(Page* page) {
    NodeHeader* nh = GetNodeHeader(page);
    std::memset(nh, 0, sizeof(NodeHeader));
    nh->is_leaf = 1;
    nh->key_count = 0;
    nh->parent = INVALID_PAGE_ID;
    nh->next = INVALID_PAGE_ID;
    nh->prev = INVALID_PAGE_ID;
}

void BPlusTree::InitializeInternal(Page* page) {
    NodeHeader* nh = GetNodeHeader(page);
    std::memset(nh, 0, sizeof(NodeHeader));
    nh->is_leaf = 0;
    nh->key_count = 0;
    nh->parent = INVALID_PAGE_ID;
}

BPlusTree::InternalArrays BPlusTree::GetInternalArrays(Page* page) {
    char* base = PagePayload(page) + NodeHeaderSize;
    // 简单分割：前半是 children（key_count+1），后半是 keys（key_count）
    // 为避免动态计算，这里用最大容量切片视图
    size_t payload = PAGE_SIZE - PAGE_HEADER_SIZE - NodeHeaderSize;
    // 预留一半给 children，一半给 keys（教学简化，真实实现需紧凑布局）
    size_t half = payload / 2;
    InternalArrays ia{};
    ia.children = reinterpret_cast<page_id_t*>(base);
    ia.keys = reinterpret_cast<int32_t*>(base + half);
    return ia;
}

const BPlusTree::InternalArrays BPlusTree::GetInternalArraysConst(const Page* page) {
    char* base = const_cast<char*>(page->GetData()) + PAGE_HEADER_SIZE + NodeHeaderSize;
    size_t payload = PAGE_SIZE - PAGE_HEADER_SIZE - NodeHeaderSize;
    size_t half = payload / 2;
    InternalArrays ia{};
    ia.children = reinterpret_cast<page_id_t*>(base);
    ia.keys = reinterpret_cast<int32_t*>(base + half);
    return ia;
}

uint16_t BPlusTree::GetInternalMaxKeys() {
    size_t payload = PAGE_SIZE - PAGE_HEADER_SIZE - NodeHeaderSize;
    size_t half = payload / 2;
    // half / sizeof(page_id_t) 给 children 个数≈max_keys+1；另一半给 keys
    size_t max_keys = std::min( (half / sizeof(int32_t)), (half / sizeof(page_id_t)) - 1 );
    return static_cast<uint16_t>(max_keys);
}

Page* BPlusTree::DescendToLeaf(int32_t key) {
    if (root_page_id_ == INVALID_PAGE_ID) return nullptr;
    Page* p = engine_->GetPage(root_page_id_);
    if (!p) return nullptr;
    const NodeHeader* nh = GetNodeHeaderConst(p);
    if (nh->is_leaf == 1) {
        return p; // caller负责 PutPage
    }
    // 内节点一层：根据 keys 选择 child
    auto ia = GetInternalArraysConst(p);
    uint16_t n = nh->key_count;
    uint16_t i = 0;
    while (i < n && key >= ia.keys[i]) ++i;
    page_id_t child = ia.children[i];
    engine_->PutPage(p->GetPageId(), false);
    Page* leaf = engine_->GetPage(child);
    return leaf;
}

void BPlusTree::PromoteNewRoot(Page* left_leaf, Page* right_leaf, int32_t separator_key) {
    // root 可能是旧叶子，创建新根（internal）
    page_id_t new_root_id = INVALID_PAGE_ID;
    Page* root = engine_->CreatePage(&new_root_id);
    if (!root) return;
    root->InitializePage(PageType::INDEX_PAGE);
    InitializeInternal(root);
    NodeHeader* rnh = GetNodeHeader(root);
    auto ia = GetInternalArrays(root);
    // children: [left, right]; keys: [separator]
    ia.children[0] = left_leaf->GetPageId();
    ia.children[1] = right_leaf->GetPageId();
    ia.keys[0] = separator_key;
    rnh->key_count = 1;
    // 更新叶子父指针
    NodeHeader* lnh = GetNodeHeader(left_leaf);
    NodeHeader* rnh_leaf = GetNodeHeader(right_leaf);
    lnh->parent = new_root_id;
    rnh_leaf->parent = new_root_id;
    engine_->PutPage(root->GetPageId(), true);
    root_page_id_ = new_root_id;
}

bool BPlusTree::InsertIntoLeafAndSplitIfNeeded(Page* leaf, int32_t key, const RID& rid) {
    NodeHeader* nh = GetNodeHeader(leaf);
    uint16_t n = nh->key_count;
    uint16_t cap = GetLeafMaxEntries();
    LeafEntry* arr = GetLeafEntries(leaf);
    // 找位置
    uint16_t pos = 0;
    while (pos < n && arr[pos].key < key) ++pos;
    if (pos < n && arr[pos].key == key) {
        arr[pos].rid_page = rid.page_id;
        arr[pos].rid_slot = rid.slot;
        engine_->PutPage(leaf->GetPageId(), true);
        return true;
    }
    if (n < cap) {
        for (uint16_t i = n; i > pos; --i) arr[i] = arr[i-1];
        arr[pos].key = key; arr[pos].rid_page = rid.page_id; arr[pos].rid_slot = rid.slot; arr[pos].pad = 0;
        nh->key_count = static_cast<uint16_t>(n + 1);
        engine_->PutPage(leaf->GetPageId(), true);
        return true;
    }
    // 分裂：一半移动到新叶
    page_id_t new_leaf_id = INVALID_PAGE_ID;
    Page* new_leaf = engine_->CreatePage(&new_leaf_id);
    if (!new_leaf) { engine_->PutPage(leaf->GetPageId(), false); return false; }
    new_leaf->InitializePage(PageType::INDEX_PAGE);
    InitializeLeaf(new_leaf);
    LeafEntry* arr_new = GetLeafEntries(new_leaf);
    uint16_t left_sz = n / 2;
    uint16_t right_sz = static_cast<uint16_t>(n - left_sz);
    // 先确定插入后应处于哪边，再搬迁
    std::vector<LeafEntry> tmp(n + 1);
    for (uint16_t i = 0; i < n; ++i) tmp[i] = arr[i];
    for (uint16_t i = n; i > pos; --i) tmp[i] = tmp[i-1];
    tmp[pos].key = key; tmp[pos].rid_page = rid.page_id; tmp[pos].rid_slot = rid.slot; tmp[pos].pad = 0;
    uint16_t total = static_cast<uint16_t>(n + 1);
    left_sz = total / 2;
    right_sz = static_cast<uint16_t>(total - left_sz);
    // 左边回写到原叶，右边写到新叶
    for (uint16_t i = 0; i < left_sz; ++i) arr[i] = tmp[i];
    for (uint16_t i = 0; i < right_sz; ++i) arr_new[i] = tmp[left_sz + i];
    nh->key_count = left_sz;
    NodeHeader* nh_new = GetNodeHeader(new_leaf);
    nh_new->key_count = right_sz;
    // 连接兄弟指针
    nh_new->next = nh->next;
    nh_new->prev = leaf->GetPageId();
    nh->next = new_leaf_id;
    if (nh_new->next != INVALID_PAGE_ID) {
        Page* nn = engine_->GetPage(nh_new->next);
        if (nn) {
            NodeHeader* nh_nn = GetNodeHeader(nn);
            nh_nn->prev = new_leaf_id;
            engine_->PutPage(nn->GetPageId(), true);
        }
    }
    // 分隔键：右叶最小键
    int32_t sep = arr_new[0].key;
    // 根处理
    if (leaf->GetPageId() == root_page_id_) {
        PromoteNewRoot(leaf, new_leaf, sep);
    } else {
        // 最小实现不处理多层提升（需要父节点插入），暂留
    }
    engine_->PutPage(leaf->GetPageId(), true);
    engine_->PutPage(new_leaf_id, true);
    return true;
}

bool BPlusTree::Insert(int32_t key, const RID& rid) {
    if (root_page_id_ == INVALID_PAGE_ID) {
        if (CreateNew() == INVALID_PAGE_ID) return false;
    }
    Page* leaf = DescendToLeaf(key);
    if (!leaf) return false;
    bool ok = InsertIntoLeafAndSplitIfNeeded(leaf, key, rid);
    // InsertIntoLeafAndSplitIfNeeded 内部负责 PutPage
    return ok;
}

std::optional<RID> BPlusTree::Search(int32_t key) {
    if (root_page_id_ == INVALID_PAGE_ID) return std::nullopt;
    Page* leaf = DescendToLeaf(key);
    if (!leaf) return std::nullopt;
    const NodeHeader* nh = GetNodeHeaderConst(leaf);
    const LeafEntry* arr = GetLeafEntriesConst(leaf);
    uint16_t n = nh->key_count;
    for (uint16_t i = 0; i < n; ++i) {
        if (arr[i].key == key) {
            RID r{arr[i].rid_page, arr[i].rid_slot};
            engine_->PutPage(leaf->GetPageId(), false);
            return r;
        }
    }
    engine_->PutPage(leaf->GetPageId(), false);
    return std::nullopt;
}

std::vector<RID> BPlusTree::Range(int32_t low, int32_t high) {
    std::vector<RID> out;
    if (root_page_id_ == INVALID_PAGE_ID) return out;
    Page* p = engine_->GetPage(root_page_id_);
    if (!p) return out;
    const NodeHeader* nh = GetNodeHeaderConst(p);
    const LeafEntry* arr = GetLeafEntriesConst(p);
    uint16_t n = nh->key_count;
    for (uint16_t i = 0; i < n; ++i) {
        if (arr[i].key >= low && arr[i].key <= high) {
            out.push_back(RID{arr[i].rid_page, arr[i].rid_slot});
        }
    }
    engine_->PutPage(root_page_id_, false);
    return out;
}

} // namespace minidb


