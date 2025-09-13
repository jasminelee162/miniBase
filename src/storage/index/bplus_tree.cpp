#include "storage/index/bplus_tree.h"
#include "storage/page/page_header.h"
#include <cstring>
#include <algorithm>
// B+树的实现
namespace minidb
{

    static inline char *PagePayload(Page *page)
    {
        return page->GetData() + PAGE_HEADER_SIZE;
    }

    BPlusTree::BPlusTree(StorageEngine *engine) : engine_(engine) {}

    page_id_t BPlusTree::CreateNew()
    {
        page_id_t pid = INVALID_PAGE_ID;
        Page *p = engine_->CreatePage(&pid);
        if (!p)
            return INVALID_PAGE_ID;
        p->InitializePage(PageType::INDEX_PAGE);
        InitializeLeaf(p);
        engine_->PutPage(pid, true);
        root_page_id_ = pid;
        // 持久化根
        if (engine_)
            engine_->SetIndexRoot(root_page_id_);
        return pid;
    }

    void BPlusTree::SetRoot(page_id_t root_id)
    {
        root_page_id_ = root_id;
        if (engine_)
            engine_->SetIndexRoot(root_page_id_);
    }

    void BPlusTree::LoadRootFromStorage()
    {
        if (!engine_)
            return;
        page_id_t saved = engine_->GetIndexRoot();
        if (saved != INVALID_PAGE_ID)
            root_page_id_ = saved;
    }

    BPlusTree::NodeHeader *BPlusTree::GetNodeHeader(Page *page)
    {
        return reinterpret_cast<NodeHeader *>(PagePayload(page));
    }

    const BPlusTree::NodeHeader *BPlusTree::GetNodeHeaderConst(const Page *page)
    {
        return reinterpret_cast<const NodeHeader *>(page->GetData() + PAGE_HEADER_SIZE);
    }

    BPlusTree::LeafEntry *BPlusTree::GetLeafEntries(Page *page)
    {
        return reinterpret_cast<LeafEntry *>(PagePayload(page) + NodeHeaderSize);
    }

    const BPlusTree::LeafEntry *BPlusTree::GetLeafEntriesConst(const Page *page)
    {
        return reinterpret_cast<const LeafEntry *>(page->GetData() + PAGE_HEADER_SIZE + NodeHeaderSize);
    }

    uint16_t BPlusTree::GetLeafMaxEntries()
    {
        size_t payload = PAGE_SIZE - PAGE_HEADER_SIZE;
        if (payload < NodeHeaderSize)
            return 0;
        return static_cast<uint16_t>((payload - NodeHeaderSize) / LeafEntrySize);
    }

    void BPlusTree::InitializeLeaf(Page *page)
    {
        NodeHeader *nh = GetNodeHeader(page);
        std::memset(nh, 0, sizeof(NodeHeader));
        nh->is_leaf = 1;
        nh->key_count = 0;
        nh->parent = INVALID_PAGE_ID;
        nh->next = INVALID_PAGE_ID;
        nh->prev = INVALID_PAGE_ID;
    }

    void BPlusTree::InitializeInternal(Page *page)
    {
        NodeHeader *nh = GetNodeHeader(page);
        std::memset(nh, 0, sizeof(NodeHeader));
        nh->is_leaf = 0;
        nh->key_count = 0;
        nh->parent = INVALID_PAGE_ID;
    }

    BPlusTree::InternalArrays BPlusTree::GetInternalArrays(Page *page)
    {
        char *base = PagePayload(page) + NodeHeaderSize;
        // 简单分割：前半是 children（key_count+1），后半是 keys（key_count）
        // 为避免动态计算，这里用最大容量切片视图
        size_t payload = PAGE_SIZE - PAGE_HEADER_SIZE - NodeHeaderSize;
        // 预留一半给 children，一半给 keys（教学简化，真实实现需紧凑布局）
        size_t half = payload / 2;
        InternalArrays ia{};
        ia.children = reinterpret_cast<page_id_t *>(base);
        ia.keys = reinterpret_cast<int32_t *>(base + half);
        return ia;
    }

    const BPlusTree::InternalArrays BPlusTree::GetInternalArraysConst(const Page *page)
    {
        char *base = const_cast<char *>(page->GetData()) + PAGE_HEADER_SIZE + NodeHeaderSize;
        size_t payload = PAGE_SIZE - PAGE_HEADER_SIZE - NodeHeaderSize;
        size_t half = payload / 2;
        InternalArrays ia{};
        ia.children = reinterpret_cast<page_id_t *>(base);
        ia.keys = reinterpret_cast<int32_t *>(base + half);
        return ia;
    }

    uint16_t BPlusTree::GetInternalMaxKeys()
    {
        size_t payload = PAGE_SIZE - PAGE_HEADER_SIZE - NodeHeaderSize;
        size_t half = payload / 2;
        // half / sizeof(page_id_t) 给 children 个数≈max_keys+1；另一半给 keys
        size_t max_keys = std::min((half / sizeof(int32_t)), (half / sizeof(page_id_t)) - 1);
        return static_cast<uint16_t>(max_keys);
    }

    Page *BPlusTree::DescendToLeaf(int32_t key)
    {
        if (root_page_id_ == INVALID_PAGE_ID)
            return nullptr;
        Page *p = engine_->GetPage(root_page_id_);
        while (p)
        {
            const NodeHeader *nh = GetNodeHeaderConst(p);
            if (nh->is_leaf == 1)
            {
                return p; // caller负责 PutPage
            }
            // 根据 keys 选择下一子节点
            auto ia = GetInternalArraysConst(p);
            uint16_t n = nh->key_count;
            uint16_t i = 0;
            while (i < n && key >= ia.keys[i])
                ++i;
            page_id_t child = ia.children[i];
            page_id_t cur = p->GetPageId();
            engine_->PutPage(cur, false);
            p = engine_->GetPage(child);
        }
        return nullptr;
    }

    void BPlusTree::PromoteNewRoot(Page *left_leaf, Page *right_leaf, int32_t separator_key)
    {
        // root 可能是旧叶子，创建新根（internal）
        page_id_t new_root_id = INVALID_PAGE_ID;
        Page *root = engine_->CreatePage(&new_root_id);
        if (!root)
            return;
        root->InitializePage(PageType::INDEX_PAGE);
        InitializeInternal(root);
        NodeHeader *rnh = GetNodeHeader(root);
        auto ia = GetInternalArrays(root);
        // children: [left, right]; keys: [separator]
        ia.children[0] = left_leaf->GetPageId();
        ia.children[1] = right_leaf->GetPageId();
        ia.keys[0] = separator_key;
        rnh->key_count = 1;
        // 更新叶子父指针
        NodeHeader *lnh = GetNodeHeader(left_leaf);
        NodeHeader *rnh_leaf = GetNodeHeader(right_leaf);
        lnh->parent = new_root_id;
        rnh_leaf->parent = new_root_id;
        engine_->PutPage(root->GetPageId(), true);
        root_page_id_ = new_root_id;
    }

    int BPlusTree::FindChildIndex(Page *parent, page_id_t left_child_id)
    {
        const NodeHeader *nh = GetNodeHeaderConst(parent);
        auto ia = GetInternalArraysConst(parent);
        // children 数组大小 = key_count + 1
        for (uint16_t i = 0; i < nh->key_count + 1; ++i)
        {
            if (ia.children[i] == left_child_id)
                return static_cast<int>(i);
        }
        return -1;
    }

    bool BPlusTree::InsertIntoInternal(Page *parent, int insert_pos, int32_t key, page_id_t right_child)
    {
        NodeHeader *nh = GetNodeHeader(parent);
        auto ia = GetInternalArrays(parent);
        uint16_t n = nh->key_count;
        if (n >= GetInternalMaxKeys())
            return false; // 需要分裂
        // keys 右移，从 insert_pos 开始
        for (int i = static_cast<int>(n); i > insert_pos; --i)
        {
            ia.keys[i] = ia.keys[i - 1];
        }
        // children 右移，从 insert_pos+1 开始
        for (int i = static_cast<int>(n + 1); i > insert_pos + 1; --i)
        {
            ia.children[i] = ia.children[i - 1];
        }
        ia.keys[insert_pos] = key;
        ia.children[insert_pos + 1] = right_child;
        nh->key_count = static_cast<uint16_t>(n + 1);
        engine_->PutPage(parent->GetPageId(), true);
        return true;
    }

    void BPlusTree::SplitInternalAndPropagate(Page *parent, int32_t key, page_id_t right_child)
    {
        // 尝试直接插入，若成功则返回
        const NodeHeader *nhc = GetNodeHeaderConst(parent);
        auto iac = GetInternalArraysConst(parent);
        // 选择插入位置：keys 有序，找到第一个 >= key 的位置
        int pos = 0;
        while (pos < nhc->key_count && key > iac.keys[pos])
            ++pos;
        if (InsertIntoInternal(parent, pos, key, right_child))
            return;

        // 分裂
        NodeHeader *nh = GetNodeHeader(parent);
        auto ia = GetInternalArrays(parent);
        uint16_t n = nh->key_count;

        // 临时数组：把新键插入后再切分
        std::vector<int32_t> keys(n + 1);
        std::vector<page_id_t> children(n + 2);
        for (uint16_t i = 0; i < n; ++i)
            keys[i] = ia.keys[i];
        for (uint16_t i = 0; i < n + 1; ++i)
            children[i] = ia.children[i];
        int insert_pos = 0;
        while (insert_pos < n && key > keys[insert_pos])
            ++insert_pos;
        for (int i = static_cast<int>(n); i > insert_pos; --i)
            keys[i] = keys[i - 1];
        for (int i = static_cast<int>(n + 1); i > insert_pos + 1; --i)
            children[i] = children[i - 1];
        keys[insert_pos] = key;
        children[insert_pos + 1] = right_child;

        // 选择分隔键：中位
        uint16_t total_keys = static_cast<uint16_t>(n + 1);
        uint16_t mid = total_keys / 2; // 提升 keys[mid]
        int32_t up_key = keys[mid];

        // 左节点回写 keys[0..mid-1] 与 children[0..mid]
        for (uint16_t i = 0; i < mid; ++i)
            ia.keys[i] = keys[i];
        for (uint16_t i = 0; i < mid + 1; ++i)
            ia.children[i] = children[i];
        nh->key_count = mid;

        // 创建右内节点
        page_id_t right_id = INVALID_PAGE_ID;
        Page *right_node = engine_->CreatePage(&right_id);
        if (!right_node)
        {
            engine_->PutPage(parent->GetPageId(), true);
            return;
        }
        right_node->InitializePage(PageType::INDEX_PAGE);
        InitializeInternal(right_node);
        NodeHeader *rnh = GetNodeHeader(right_node);
        auto ria = GetInternalArrays(right_node);
        uint16_t right_keys = static_cast<uint16_t>(total_keys - mid - 1);
        // 填充右节点：keys[mid+1..]，children[mid+1..]
        for (uint16_t i = 0; i < right_keys; ++i)
            ria.keys[i] = keys[mid + 1 + i];
        for (uint16_t i = 0; i < right_keys + 1; ++i)
            ria.children[i] = children[mid + 1 + i];
        rnh->key_count = right_keys;

        // 更新子节点父指针
        for (uint16_t i = 0; i < right_keys + 1; ++i)
        {
            Page *ch = engine_->GetPage(ria.children[i]);
            if (ch)
            {
                NodeHeader *chnh = GetNodeHeader(ch);
                chnh->parent = right_id;
                engine_->PutPage(ch->GetPageId(), true);
            }
        }
        // 左节点已在当前 parent 页面，父指针保持不变
        engine_->PutPage(parent->GetPageId(), true);
        engine_->PutPage(right_id, true);

        // 将 up_key 插入到 parent 的父节点
        InsertIntoParent(parent->GetPageId(), right_id, up_key);
    }

    void BPlusTree::InsertIntoParent(page_id_t left_child, page_id_t right_child, int32_t separator_key)
    {
        // 如果 left_child 是根，则需要创建新根
        if (left_child == root_page_id_)
        {
            Page *left = engine_->GetPage(left_child);
            Page *right = engine_->GetPage(right_child);
            if (!left || !right)
                return;
            PromoteNewRoot(left, right, separator_key);
            engine_->PutPage(left_child, false);
            engine_->PutPage(right_child, false);
            return;
        }
        // 否则找到父节点并尝试插入，必要时分裂
        // 通过任一子节点的 parent 指针找到父
        Page *left = engine_->GetPage(left_child);
        if (!left)
            return;
        page_id_t parent_id = GetNodeHeader(left)->parent;
        engine_->PutPage(left_child, false);
        if (parent_id == INVALID_PAGE_ID)
            return;
        Page *parent = engine_->GetPage(parent_id);
        if (!parent)
            return;
        int idx = FindChildIndex(parent, left_child);
        if (idx < 0)
        {
            engine_->PutPage(parent_id, false);
            return;
        }
        // 尝试直接在 idx 位置插入（key 放在 children[idx] 与 children[idx+1] 之间）
        if (!InsertIntoInternal(parent, idx, separator_key, right_child))
        {
            // 需要分裂
            SplitInternalAndPropagate(parent, separator_key, right_child);
        }
        else
        {
            // 更新右子父指针
            Page *rc = engine_->GetPage(right_child);
            if (rc)
            {
                GetNodeHeader(rc)->parent = parent_id;
                engine_->PutPage(right_child, true);
            }
        }
        engine_->PutPage(parent_id, true);
    }

    bool BPlusTree::InsertIntoLeafAndSplitIfNeeded(Page *leaf, int32_t key, const RID &rid)
    {
        NodeHeader *nh = GetNodeHeader(leaf);
        uint16_t n = nh->key_count;
        uint16_t cap = GetLeafMaxEntries();
        LeafEntry *arr = GetLeafEntries(leaf);
        // 找位置
        uint16_t pos = 0;
        while (pos < n && arr[pos].key < key)
            ++pos;
        if (pos < n && arr[pos].key == key)
        {
            arr[pos].rid_page = rid.page_id;
            arr[pos].rid_slot = rid.slot;
            engine_->PutPage(leaf->GetPageId(), true);
            return true;
        }
        if (n < cap)
        {
            for (uint16_t i = n; i > pos; --i)
                arr[i] = arr[i - 1];
            arr[pos].key = key;
            arr[pos].rid_page = rid.page_id;
            arr[pos].rid_slot = rid.slot;
            arr[pos].pad = 0;
            nh->key_count = static_cast<uint16_t>(n + 1);
            engine_->PutPage(leaf->GetPageId(), true);
            return true;
        }
        // 分裂：一半移动到新叶
        page_id_t new_leaf_id = INVALID_PAGE_ID;
        Page *new_leaf = engine_->CreatePage(&new_leaf_id);
        if (!new_leaf)
        {
            engine_->PutPage(leaf->GetPageId(), false);
            return false;
        }
        new_leaf->InitializePage(PageType::INDEX_PAGE);
        InitializeLeaf(new_leaf);
        LeafEntry *arr_new = GetLeafEntries(new_leaf);
        uint16_t left_sz = n / 2;
        uint16_t right_sz = static_cast<uint16_t>(n - left_sz);
        // 先确定插入后应处于哪边，再搬迁
        std::vector<LeafEntry> tmp(n + 1);
        for (uint16_t i = 0; i < n; ++i)
            tmp[i] = arr[i];
        for (uint16_t i = n; i > pos; --i)
            tmp[i] = tmp[i - 1];
        tmp[pos].key = key;
        tmp[pos].rid_page = rid.page_id;
        tmp[pos].rid_slot = rid.slot;
        tmp[pos].pad = 0;
        uint16_t total = static_cast<uint16_t>(n + 1);
        left_sz = total / 2;
        right_sz = static_cast<uint16_t>(total - left_sz);
        // 左边回写到原叶，右边写到新叶
        for (uint16_t i = 0; i < left_sz; ++i)
            arr[i] = tmp[i];
        for (uint16_t i = 0; i < right_sz; ++i)
            arr_new[i] = tmp[left_sz + i];
        nh->key_count = left_sz;
        NodeHeader *nh_new = GetNodeHeader(new_leaf);
        nh_new->key_count = right_sz;
        // 连接兄弟指针
        nh_new->next = nh->next;
        nh_new->prev = leaf->GetPageId();
        nh->next = new_leaf_id;
        if (nh_new->next != INVALID_PAGE_ID)
        {
            Page *nn = engine_->GetPage(nh_new->next);
            if (nn)
            {
                NodeHeader *nh_nn = GetNodeHeader(nn);
                nh_nn->prev = new_leaf_id;
                engine_->PutPage(nn->GetPageId(), true);
            }
        }
        // 分隔键：右叶最小键
        int32_t sep = arr_new[0].key;
        // 根处理或向上插入
        if (leaf->GetPageId() == root_page_id_)
        {
            PromoteNewRoot(leaf, new_leaf, sep);
        }
        else
        {
            // 将分隔键插入到父节点（可能级联分裂）
            InsertIntoParent(leaf->GetPageId(), new_leaf_id, sep);
        }
        engine_->PutPage(leaf->GetPageId(), true);
        engine_->PutPage(new_leaf_id, true);
        return true;
    }

    bool BPlusTree::Insert(int32_t key, const RID &rid)
    {
        if (root_page_id_ == INVALID_PAGE_ID)
        {
            if (CreateNew() == INVALID_PAGE_ID)
                return false;
        }
        Page *leaf = DescendToLeaf(key);
        if (!leaf)
            return false;
        bool ok = InsertIntoLeafAndSplitIfNeeded(leaf, key, rid);
        // InsertIntoLeafAndSplitIfNeeded 内部负责 PutPage
        return ok;
    }

    std::optional<RID> BPlusTree::Search(int32_t key)
    {
        if (root_page_id_ == INVALID_PAGE_ID)
            return std::nullopt;
        Page *leaf = DescendToLeaf(key);
        if (!leaf)
            return std::nullopt;
        const NodeHeader *nh = GetNodeHeaderConst(leaf);
        const LeafEntry *arr = GetLeafEntriesConst(leaf);
        uint16_t n = nh->key_count;
        for (uint16_t i = 0; i < n; ++i)
        {
            if (arr[i].key == key)
            {
                RID r{arr[i].rid_page, arr[i].rid_slot};
                engine_->PutPage(leaf->GetPageId(), false);
                return r;
            }
        }
        engine_->PutPage(leaf->GetPageId(), false);
        return std::nullopt;
    }

    std::vector<RID> BPlusTree::Range(int32_t low, int32_t high)
    {
        std::vector<RID> out;
        if (root_page_id_ == INVALID_PAGE_ID)
            return out;
        // 先降到可能包含low的叶子
        Page *leaf = DescendToLeaf(low);
        if (!leaf)
            return out;
        page_id_t pid = leaf->GetPageId();
        while (leaf)
        {
            const NodeHeader *nh = GetNodeHeaderConst(leaf);
            const LeafEntry *arr = GetLeafEntriesConst(leaf);
            uint16_t n = nh->key_count;
            for (uint16_t i = 0; i < n; ++i)
            {
                int32_t k = arr[i].key;
                if (k > high)
                { // 超出上界：可以提前结束
                    engine_->PutPage(pid, false);
                    return out;
                }
                if (k >= low)
                {
                    out.push_back(RID{arr[i].rid_page, arr[i].rid_slot});
                }
            }
            page_id_t next = nh->next;
            engine_->PutPage(pid, false);
            if (next == INVALID_PAGE_ID)
                break;
            leaf = engine_->GetPage(next);
            pid = next;
        }
        return out;
    }

    // ===== 增强操作实现 =====

    bool BPlusTree::Delete(int32_t key)
    {
        return DeleteAndRebalance(key);
    }

    bool BPlusTree::Update(int32_t key, const RID &new_rid)
    {
        if (root_page_id_ == INVALID_PAGE_ID)
            return false;
        Page *leaf = DescendToLeaf(key);
        if (!leaf)
            return false;
        return UpdateInLeaf(leaf, key, new_rid);
    }

    bool BPlusTree::HasKey(int32_t key)
    {
        return Search(key).has_value();
    }

    size_t BPlusTree::GetKeyCount() const
    {
        if (root_page_id_ == INVALID_PAGE_ID)
            return 0;
        Page *p = engine_->GetPage(root_page_id_);
        if (!p)
            return 0;
        const NodeHeader *nh = GetNodeHeaderConst(p);
        size_t count = nh->key_count;
        engine_->PutPage(root_page_id_, false);
        return count;
    }

    // ===== 辅助方法实现 =====

    bool BPlusTree::DeleteFromLeaf(Page *leaf, int32_t key)
    {
        NodeHeader *nh = GetNodeHeader(leaf);
        LeafEntry *arr = GetLeafEntries(leaf);
        uint16_t n = nh->key_count;

        // 查找要删除的键
        int32_t index = FindKeyIndex(arr, n, key);
        if (index == -1)
        {
            engine_->PutPage(leaf->GetPageId(), false);
            return false; // 键不存在
        }

        // 移动后续元素
        for (uint16_t i = index; i < n - 1; ++i)
        {
            arr[i] = arr[i + 1];
        }
        nh->key_count--;

        engine_->PutPage(leaf->GetPageId(), true);
        return true;
    }

    int32_t BPlusTree::GetLeafFirstKey(const Page *leaf) const
    {
        const NodeHeader *nh = GetNodeHeaderConst(leaf);
        if (nh->key_count == 0)
            return INT32_MIN;
        const LeafEntry *arr = GetLeafEntriesConst(leaf);
        return arr[0].key;
    }

    void BPlusTree::RemoveParentKey(page_id_t parent_id, int key_index)
    {
        Page *p = engine_->GetPage(parent_id);
        if (!p)
            return;
        NodeHeader *nh = GetNodeHeader(p);
        auto ia = GetInternalArrays(p);
        uint16_t n = nh->key_count;
        for (int i = key_index; i < static_cast<int>(n) - 1; ++i)
            ia.keys[i] = ia.keys[i + 1];
        for (int i = key_index + 1; i < static_cast<int>(n); ++i)
            ia.children[i] = ia.children[i + 1];
        nh->key_count = static_cast<uint16_t>(n - 1);
        engine_->PutPage(parent_id, true);
    }

    void BPlusTree::ReplaceParentKey(page_id_t parent_id, int key_index, int32_t new_key)
    {
        Page *p = engine_->GetPage(parent_id);
        if (!p)
            return;
        auto ia = GetInternalArrays(p);
        ia.keys[key_index] = new_key;
        engine_->PutPage(parent_id, true);
    }

    void BPlusTree::RebalanceLeaf(Page *leaf)
    {
        NodeHeader *nh = GetNodeHeader(leaf);
        uint16_t min_entries = GetLeafMinEntries();
        if (nh->key_count >= min_entries || leaf->GetPageId() == root_page_id_)
            return;
        page_id_t parent_id = nh->parent;
        if (parent_id == INVALID_PAGE_ID)
            return;
        Page *parent = engine_->GetPage(parent_id);
        if (!parent)
            return;
        auto ia = GetInternalArrays(parent);
        int idx = FindChildIndex(parent, leaf->GetPageId());
        if (idx < 0)
        {
            engine_->PutPage(parent_id, false);
            return;
        }
        // 选择左兄弟或右兄弟
        page_id_t left_sib_id = (idx > 0) ? ia.children[idx - 1] : INVALID_PAGE_ID;
        page_id_t right_sib_id = (idx + 1 <= GetNodeHeaderConst(parent)->key_count) ? ia.children[idx + 1] : INVALID_PAGE_ID;

        // 尝试从左兄弟借一个最大键
        if (left_sib_id != INVALID_PAGE_ID)
        {
            Page *left = engine_->GetPage(left_sib_id);
            NodeHeader *lnh = GetNodeHeader(left);
            if (lnh->key_count > min_entries)
            {
                LeafEntry *larr = GetLeafEntries(left);
                LeafEntry *arr = GetLeafEntries(leaf);
                // 将 leaf 中元素整体右移一位
                for (int i = nh->key_count; i > 0; --i)
                    arr[i] = arr[i - 1];
                arr[0] = larr[lnh->key_count - 1];
                nh->key_count++;
                lnh->key_count--;
                // 父分隔键替换为 leaf 新首键
                ReplaceParentKey(parent_id, idx - 1, arr[0].key);
                engine_->PutPage(left_sib_id, true);
                engine_->PutPage(leaf->GetPageId(), true);
                engine_->PutPage(parent_id, true);
                return;
            }
            engine_->PutPage(left_sib_id, false);
        }
        // 尝试从右兄弟借一个最小键
        if (right_sib_id != INVALID_PAGE_ID)
        {
            Page *right = engine_->GetPage(right_sib_id);
            NodeHeader *rnh = GetNodeHeader(right);
            if (rnh->key_count > min_entries)
            {
                LeafEntry *rarr = GetLeafEntries(right);
                LeafEntry *arr = GetLeafEntries(leaf);
                // 借用 right 的最小键到 leaf 尾部
                for (uint16_t i = nh->key_count; i > 0; --i)
                { /* keep order */
                }
                arr[nh->key_count] = rarr[0];
                nh->key_count++;
                // 右 sibling 左移
                for (uint16_t i = 0; i < rnh->key_count - 1; ++i)
                    rarr[i] = rarr[i + 1];
                rnh->key_count--;
                // 父分隔键替换为 right 新首键
                ReplaceParentKey(parent_id, idx, rarr[0].key);
                engine_->PutPage(right_sib_id, true);
                engine_->PutPage(leaf->GetPageId(), true);
                engine_->PutPage(parent_id, true);
                return;
            }
            engine_->PutPage(right_sib_id, false);
        }
        // 两侧都无法借，执行合并：优先与左合并，否则与右合并
        if (left_sib_id != INVALID_PAGE_ID)
        {
            Page *left = engine_->GetPage(left_sib_id);
            LeafEntry *larr = GetLeafEntries(left);
            LeafEntry *arr = GetLeafEntries(leaf);
            NodeHeader *lnh = GetNodeHeader(left);
            // 将 leaf 全部搬到 left 尾部
            for (uint16_t i = 0; i < nh->key_count; ++i)
                larr[lnh->key_count + i] = arr[i];
            lnh->key_count = static_cast<uint16_t>(lnh->key_count + nh->key_count);
            // 调整链表
            lnh->next = nh->next;
            if (nh->next != INVALID_PAGE_ID)
            {
                Page *nxt = engine_->GetPage(nh->next);
                if (nxt)
                {
                    GetNodeHeader(nxt)->prev = left_sib_id;
                    engine_->PutPage(nxt->GetPageId(), true);
                }
            }
            engine_->PutPage(left_sib_id, true);
            // 在父节点删除分隔键 idx-1，并移除 children[idx]
            RemoveParentKey(parent_id, idx - 1);
            engine_->PutPage(parent_id, true);
            engine_->PutPage(leaf->GetPageId(), false);
            return;
        }
        if (right_sib_id != INVALID_PAGE_ID)
        {
            Page *right = engine_->GetPage(right_sib_id);
            LeafEntry *rarr = GetLeafEntries(right);
            LeafEntry *arr = GetLeafEntries(leaf);
            NodeHeader *rnh = GetNodeHeader(right);
            // 将 right 全部搬到 leaf 尾部
            for (uint16_t i = 0; i < rnh->key_count; ++i)
                arr[nh->key_count + i] = rarr[i];
            nh->key_count = static_cast<uint16_t>(nh->key_count + rnh->key_count);
            // 调整链表
            nh->next = rnh->next;
            if (rnh->next != INVALID_PAGE_ID)
            {
                Page *nxt = engine_->GetPage(rnh->next);
                if (nxt)
                {
                    GetNodeHeader(nxt)->prev = leaf->GetPageId();
                    engine_->PutPage(nxt->GetPageId(), true);
                }
            }
            engine_->PutPage(leaf->GetPageId(), true);
            // 在父节点删除分隔键 idx，并移除 children[idx+1]
            RemoveParentKey(parent_id, idx);
            engine_->PutPage(parent_id, true);
            engine_->PutPage(right_sib_id, false);
            return;
        }
        engine_->PutPage(parent_id, false);
    }

<<<<<<< HEAD

// ===== 增强操作实现 =====
=======
    void BPlusTree::RebalanceInternal(page_id_t node_id)
    {
        Page *node = engine_->GetPage(node_id);
        if (!node)
            return;
        NodeHeader *nh = GetNodeHeader(node);
        if (node_id == root_page_id_)
        {
            // 根若无键且有一个孩子，下降根
            if (nh->key_count == 0)
            {
                auto ia = GetInternalArrays(node);
                root_page_id_ = ia.children[0];
                Page *child = engine_->GetPage(root_page_id_);
                if (child)
                {
                    GetNodeHeader(child)->parent = INVALID_PAGE_ID;
                    engine_->PutPage(root_page_id_, true);
                }
            }
            engine_->PutPage(node_id, true);
            return;
        }
        uint16_t min_keys = GetInternalMinKeys();
        if (nh->key_count >= min_keys)
        {
            engine_->PutPage(node_id, false);
            return;
        }
        page_id_t parent_id = nh->parent;
        Page *parent = engine_->GetPage(parent_id);
        if (!parent)
        {
            engine_->PutPage(node_id, false);
            return;
        }
        int idx = FindChildIndex(parent, node_id);
        if (idx < 0)
        {
            engine_->PutPage(parent_id, false);
            engine_->PutPage(node_id, false);
            return;
        }
        auto pia = GetInternalArrays(parent);
        page_id_t left_id = (idx > 0) ? pia.children[idx - 1] : INVALID_PAGE_ID;
        page_id_t right_id = (idx + 1 <= GetNodeHeaderConst(parent)->key_count) ? pia.children[idx + 1] : INVALID_PAGE_ID;
>>>>>>> a4c8805e3bca4ca532b01c648f73621ca3321ab2

        // 尝试从左借
        if (left_id != INVALID_PAGE_ID)
        {
            Page *left = engine_->GetPage(left_id);
            NodeHeader *lnh = GetNodeHeader(left);
            if (lnh->key_count > min_keys)
            {
                auto lia = GetInternalArrays(left);
                auto ia = GetInternalArrays(node);
                // 将 parent 分隔键下移到 node 开头，left 最大 child 上移为 parent 分隔键
                // 右移 node 的 keys/children
                for (int i = nh->key_count; i > 0; --i)
                    ia.keys[i] = ia.keys[i - 1];
                for (int i = nh->key_count + 1; i > 0; --i)
                    ia.children[i] = ia.children[i - 1];
                ia.children[0] = lia.children[lnh->key_count];
                ia.keys[0] = GetInternalArraysConst(parent).keys[idx - 1];
                nh->key_count++;
                // 提升 left 的最大键到 parent
                ReplaceParentKey(parent_id, idx - 1, lia.keys[lnh->key_count - 1]);
                lnh->key_count--;
                engine_->PutPage(left_id, true);
                engine_->PutPage(node_id, true);
                engine_->PutPage(parent_id, true);
                return;
            }
            engine_->PutPage(left_id, false);
        }
        // 尝试从右借
        if (right_id != INVALID_PAGE_ID)
        {
            Page *right = engine_->GetPage(right_id);
            NodeHeader *rnh = GetNodeHeader(right);
            if (rnh->key_count > min_keys)
            {
                auto ria = GetInternalArrays(right);
                auto ia = GetInternalArrays(node);
                // 将 parent 分隔键下移到 node 末尾，right 最小 child 上移为 parent 分隔键
                ia.keys[nh->key_count] = GetInternalArraysConst(parent).keys[idx];
                ia.children[nh->key_count + 1] = ria.children[0];
                nh->key_count++;
                // right 左移
                ReplaceParentKey(parent_id, idx, ria.keys[0]);
                for (uint16_t i = 0; i < rnh->key_count - 1; ++i)
                    ria.keys[i] = ria.keys[i + 1];
                for (uint16_t i = 0; i < rnh->key_count; ++i)
                    ria.children[i] = ria.children[i + 1];
                rnh->key_count--;
                engine_->PutPage(right_id, true);
                engine_->PutPage(node_id, true);
                engine_->PutPage(parent_id, true);
                return;
            }
            engine_->PutPage(right_id, false);
        }
        // 合并：与左或右合并
        if (left_id != INVALID_PAGE_ID)
        {
            Page *left = engine_->GetPage(left_id);
            auto lia = GetInternalArrays(left);
            auto ia = GetInternalArrays(node);
            uint16_t ln = GetNodeHeaderConst(left)->key_count;
            // 将 parent 分隔键作为中间键插到 left 末尾
            lia.keys[ln] = GetInternalArraysConst(parent).keys[idx - 1];
            for (uint16_t i = 0; i < nh->key_count; ++i)
                lia.keys[ln + 1 + i] = ia.keys[i];
            for (uint16_t i = 0; i < nh->key_count + 1; ++i)
                lia.children[ln + 1 + i] = ia.children[i];
            GetNodeHeader(left)->key_count = static_cast<uint16_t>(ln + 1 + nh->key_count);
            // 更新合并孩子的父指针
            for (uint16_t i = 0; i < nh->key_count + 1; ++i)
            {
                Page *ch = engine_->GetPage(ia.children[i]);
                if (ch)
                {
                    GetNodeHeader(ch)->parent = left_id;
                    engine_->PutPage(ia.children[i], true);
                }
            }
            RemoveParentKey(parent_id, idx - 1);
            engine_->PutPage(left_id, true);
            engine_->PutPage(node_id, false);
            // 父可能欠载，递归处理
            RebalanceInternal(parent_id);
            return;
        }
        if (right_id != INVALID_PAGE_ID)
        {
            Page *right = engine_->GetPage(right_id);
            auto ria = GetInternalArrays(right);
            auto ia = GetInternalArrays(node);
            uint16_t rn = GetNodeHeaderConst(right)->key_count;
            // 将 parent 分隔键作为中间键插到 node 末尾，再接上右节点
            ia.keys[nh->key_count] = GetInternalArraysConst(parent).keys[idx];
            for (uint16_t i = 0; i < rn; ++i)
                ia.keys[nh->key_count + 1 + i] = ria.keys[i];
            for (uint16_t i = 0; i < rn + 1; ++i)
                ia.children[nh->key_count + 1 + i] = ria.children[i];
            nh->key_count = static_cast<uint16_t>(nh->key_count + 1 + rn);
            for (uint16_t i = 0; i < rn + 1; ++i)
            {
                Page *ch = engine_->GetPage(ria.children[i]);
                if (ch)
                {
                    GetNodeHeader(ch)->parent = node_id;
                    engine_->PutPage(ria.children[i], true);
                }
            }
            RemoveParentKey(parent_id, idx);
            engine_->PutPage(node_id, true);
            engine_->PutPage(right_id, false);
            RebalanceInternal(parent_id);
            return;
        }
        engine_->PutPage(parent_id, false);
        engine_->PutPage(node_id, false);
    }

    bool BPlusTree::DeleteAndRebalance(int32_t key)
    {
        if (root_page_id_ == INVALID_PAGE_ID)
            return false;
        Page *leaf = DescendToLeaf(key);
        if (!leaf)
            return false;
        if (!DeleteFromLeaf(leaf, key))
            return false;
        // 删除后重平衡叶子
        RebalanceLeaf(leaf);
        engine_->PutPage(leaf->GetPageId(), false);
        return true;
    }

    bool BPlusTree::UpdateInLeaf(Page *leaf, int32_t key, const RID &new_rid)
    {
        NodeHeader *nh = GetNodeHeader(leaf);
        LeafEntry *arr = GetLeafEntries(leaf);
        uint16_t n = nh->key_count;

        // 查找要更新的键
        int32_t index = FindKeyIndex(arr, n, key);
        if (index == -1)
        {
            engine_->PutPage(leaf->GetPageId(), false);
            return false; // 键不存在
        }

        // 更新RID
        arr[index].rid_page = new_rid.page_id;
        arr[index].rid_slot = new_rid.slot;

        engine_->PutPage(leaf->GetPageId(), true);
        return true;
    }

    int32_t BPlusTree::FindKeyIndex(const LeafEntry *entries, uint16_t count, int32_t key)
    {
        for (uint16_t i = 0; i < count; ++i)
        {
            if (entries[i].key == key)
            {
                return static_cast<int32_t>(i);
            }
        }
        return -1; // 未找到
    }

    // ===== 模板化操作实现 =====

    template <typename KeyType>
    bool BPlusTree::InsertGeneric(const KeyType &key, const RID &rid)
    {
        int32_t int_key = ConvertToInt32(key);
        return Insert(int_key, rid);
    }

    template <typename KeyType>
    std::optional<RID> BPlusTree::SearchGeneric(const KeyType &key)
    {
        int32_t int_key = ConvertToInt32(key);
        return Search(int_key);
    }

    template <typename KeyType>
    bool BPlusTree::DeleteGeneric(const KeyType &key)
    {
        int32_t int_key = ConvertToInt32(key);
        return Delete(int_key);
    }

    // ===== 键类型转换实现 =====

    template <typename KeyType>
    int32_t BPlusTree::ConvertToInt32(const KeyType &key)
    {
        // 默认实现：直接转换
        return static_cast<int32_t>(key);
    }

    template <>
    int32_t BPlusTree::ConvertToInt32<std::string>(const std::string &key)
    {
        // 字符串转int32：使用哈希
        std::hash<std::string> hasher;
        return static_cast<int32_t>(hasher(key));
    }

    template <typename KeyType>
    KeyType BPlusTree::ConvertFromInt32(int32_t value)
    {
        // 默认实现：直接转换
        return static_cast<KeyType>(value);
    }

    template <>
    std::string BPlusTree::ConvertFromInt32<std::string>(int32_t value)
    {
        // int32转字符串：简单转换（实际应用中需要更复杂的映射）
        return std::to_string(value);
    }

    // ===== 显式模板实例化 =====

<<<<<<< HEAD
template bool BPlusTree::InsertGeneric<std::string>(const std::string& key, const RID& rid);
template std::optional<RID> BPlusTree::SearchGeneric<std::string>(const std::string& key);
template bool BPlusTree::DeleteGeneric<std::string>(const std::string& key);

} // namespace minidb
=======
    template bool BPlusTree::InsertGeneric<int32_t>(const int32_t &key, const RID &rid);
    template std::optional<RID> BPlusTree::SearchGeneric<int32_t>(const int32_t &key);
    template bool BPlusTree::DeleteGeneric<int32_t>(const int32_t &key);
>>>>>>> a4c8805e3bca4ca532b01c648f73621ca3321ab2

    template bool BPlusTree::InsertGeneric<std::string>(const std::string &key, const RID &rid);
    template std::optional<RID> BPlusTree::SearchGeneric<std::string>(const std::string &key);
    template bool BPlusTree::DeleteGeneric<std::string>(const std::string &key);

} // namespace minidb