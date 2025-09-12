#include "storage/storage_engine.h"
#include "storage/index/bplus_tree.h"
#include "../simple_test_framework.h"
#include <iostream>

using namespace minidb;

void testBPlusTreeBasic() {
    std::cout << "\n=== Test: B+Tree basic (single-leaf) ===" << std::endl;

    StorageEngine engine("data/bplustree_test.bin", 64);
    BPlusTree idx(&engine);
    page_id_t root = idx.CreateNew();
    ASSERT_TRUE(root != INVALID_PAGE_ID);

    // Insert some keys
    ASSERT_TRUE(idx.Insert(10, {1, 1}));
    ASSERT_TRUE(idx.Insert(5, {1, 2}));
    ASSERT_TRUE(idx.Insert(20, {1, 3}));

    // Update existing key
    ASSERT_TRUE(idx.Insert(10, {2, 9}));

    // Search
    auto r1 = idx.Search(10);
    ASSERT_TRUE(r1.has_value());
    ASSERT_EQ(r1->page_id, 2);
    ASSERT_EQ(r1->slot, 9);

    auto r2 = idx.Search(5);
    ASSERT_TRUE(r2.has_value());
    ASSERT_EQ(r2->page_id, 1);
    ASSERT_EQ(r2->slot, 2);

    auto r3 = idx.Search(42);
    ASSERT_FALSE(r3.has_value());

    // Range
    auto rs = idx.Range(5, 15);
    ASSERT_EQ(rs.size(), 2); // keys 5 and 10

    std::cout << "B+Tree basic test passed!" << std::endl;
}

void testBPlusTreeLeafSplitAndRootPromote() {
    std::cout << "\n=== Test: B+Tree leaf split & root promote ===" << std::endl;

    StorageEngine engine("data/bplustree_split.bin", 64);
    BPlusTree idx(&engine);
    page_id_t root = idx.CreateNew();
    ASSERT_TRUE(root != INVALID_PAGE_ID);

    // 计算容量：尽量小数据集触发一次分裂
    // 这里不直接读内部容量，按经验插入 64 个键足以触发分裂
    const int N = 64;
    for (int i = 0; i < N; ++i) {
        ASSERT_TRUE(idx.Insert(i, {static_cast<page_id_t>(i), static_cast<uint16_t>(i)}));
    }

    // 随机抽查
    for (int k : {0, 1, 31, 32, 63}) {
        auto r = idx.Search(k);
        ASSERT_TRUE(r.has_value());
        ASSERT_EQ(static_cast<int>(r->page_id), k);
        ASSERT_EQ(static_cast<int>(r->slot), k);
    }

    // 范围查询覆盖左右叶
    auto rs = idx.Range(10, 50);
    ASSERT_TRUE(rs.size() >= 1);

    std::cout << "B+Tree split & promote test passed!" << std::endl;
}

int main() {
    SimpleTest::TestSuite suite;
    suite.addTest("BPlusTreeBasic", testBPlusTreeBasic);
    suite.addTest("BPlusTreeSplit", testBPlusTreeLeafSplitAndRootPromote);
    suite.runAll();
    return SimpleTest::TestCase::getFailed() > 0 ? 1 : 0;
}
