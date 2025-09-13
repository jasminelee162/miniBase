// tests/unit/bplus_tree_split_stress_test.cpp
#include "storage/storage_engine.h"
#include "storage/index/bplus_tree.h"
#include "../simple_test_framework.h"
#include <vector>
#include <algorithm>
#include <random>

using namespace minidb;

static std::string make_db(const char* name) {
#ifdef PROJECT_ROOT_DIR
    return std::string(PROJECT_ROOT_DIR) + std::string("/data/") + name;
#else
    return std::string(name);
#endif
}

void test_many_inserts_then_search_and_range() {
    StorageEngine engine(make_db("test_bpt_split_stress.bin"), 128);
    BPlusTree tree(&engine);

    page_id_t root = tree.CreateNew();
    ASSERT_NE(root, INVALID_PAGE_ID);

    // Prepare shuffled keys to avoid best-case layout
    const int N = 500; // 足以触发多层分裂
    std::vector<int> keys(N);
    for (int i = 0; i < N; ++i) keys[i] = i * 3 + 7; // 非连续间隔，便于检查
    std::mt19937 rng(12345);
    std::shuffle(keys.begin(), keys.end(), rng);

    // Insert all keys
    for (int k : keys) {
        RID rid{static_cast<page_id_t>(k % 97 + 1), static_cast<uint16_t>(k % 31)};
        ASSERT_TRUE(tree.Insert(k, rid));
    }

    // Verify Search for a few samples
    for (int probe : {7, 16, 28, 100, 250*3+7}) {
        auto r = tree.Search(probe);
        ASSERT_TRUE(r.has_value());
    }

    // Verify Range over a window
    int lo = 200; int hi = 600;
    auto rset = tree.Range(lo, hi);
    // Expect count of numbers of form 3x+7 in [lo,hi]
    int count = 0;
    for (int v = ((lo - 7 + 2) / 3) * 3 + 7; v <= hi; v += 3) ++count;
    EXPECT_TRUE(!rset.empty());
    EXPECT_EQ((size_t)count, rset.size());

    engine.Shutdown();
}

int main() {
    SimpleTest::TestSuite suite;
    suite.addTest("BPlusTreeSplitStress", test_many_inserts_then_search_and_range);
    suite.runAll();
    return SimpleTest::TestCase::getFailed() > 0 ? 1 : 0;
}


