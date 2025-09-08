// tests/unit/storage_test.cpp
#include "storage/storage_engine.h"
#include <gtest/gtest.h>
#include <filesystem>
#include <thread>
#include <vector>

namespace minidb {

class StorageEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_db_file_ = "test_storage.db";
        std::filesystem::remove(test_db_file_);
        storage_engine_ = std::make_unique<StorageEngine>(test_db_file_, 64);
    }
    
    void TearDown() override {
        storage_engine_.reset();
        std::filesystem::remove(test_db_file_);
    }
    
    std::string test_db_file_;
    std::unique_ptr<StorageEngine> storage_engine_;
};

TEST_F(StorageEngineTest, BasicPageOperations) {
    // 创建页面
    page_id_t page_id;
    Page* page = storage_engine_->CreatePage(&page_id);
    ASSERT_NE(page, nullptr);
    
    // 写入数据
    const char* test_data = "Hello MiniDB Storage!";
    std::memcpy(page->GetData(), test_data, strlen(test_data));
    EXPECT_TRUE(storage_engine_->PutPage(page_id, true));
    
    // 读取数据
    Page* read_page = storage_engine_->GetPage(page_id);
    ASSERT_NE(read_page, nullptr);
    EXPECT_EQ(std::memcmp(read_page->GetData(), test_data, strlen(test_data)), 0);
    storage_engine_->PutPage(page_id);
}

TEST_F(StorageEngineTest, ConcurrentAccess) {
    const int num_threads = 4;
    const int pages_per_thread = 10;
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};
    
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < pages_per_thread; ++i) {
                page_id_t page_id;
                Page* page = storage_engine_->CreatePage(&page_id);
                if (page != nullptr) {
                    // 写入线程特定数据
                    int data = t * 1000 + i;
                    std::memcpy(page->GetData(), &data, sizeof(data));
                    storage_engine_->PutPage(page_id, true);
                    success_count.fetch_add(1);
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(success_count.load(), num_threads * pages_per_thread);
}

TEST_F(StorageEngineTest, CachePerformance) {
    // 创建足够多的页面以触发替换
    std::vector<page_id_t> page_ids;
    for (int i = 0; i < 100; ++i) {
        page_id_t page_id;
        Page* page = storage_engine_->CreatePage(&page_id);
        ASSERT_NE(page, nullptr);
        page_ids.push_back(page_id);
        storage_engine_->PutPage(page_id);
    }
    
    // 重复访问前50个页面
    for (int round = 0; round < 3; ++round) {
        for (int i = 0; i < 50; ++i) {
            Page* page = storage_engine_->GetPage(page_ids[i]);
            EXPECT_NE(page, nullptr);
            storage_engine_->PutPage(page_ids[i]);
        }
    }
    
    double hit_rate = storage_engine_->GetCacheHitRate();
    EXPECT_GT(hit_rate, 0.3);  // 缓存命中率应该大于30%
    
    storage_engine_->PrintStats();
}

TEST_F(StorageEngineTest, LRUReplacementPolicy) {
    LRUReplacer replacer(2);

    replacer.Unpin(1);
    replacer.Unpin(2);
    replacer.Unpin(3);
    
    frame_id_t victim;
    EXPECT_TRUE(replacer.Victim(&victim));
    EXPECT_EQ(victim, static_cast<frame_id_t>(1));  // 修正预期淘汰顺序
    
    replacer.Pin(2);
    replacer.Unpin(3);
    EXPECT_FALSE(replacer.Victim(&victim));
    
    replacer.Unpin(2);
    EXPECT_TRUE(replacer.Victim(&victim));
    EXPECT_EQ(victim, static_cast<frame_id_t>(3));
    
    // 新增测试重复unpin场景
    replacer.Unpin(1);
    replacer.Unpin(2);
    replacer.Unpin(1);
    replacer.Unpin(3);
    
    EXPECT_TRUE(replacer.Victim(&victim));
    EXPECT_EQ(victim, static_cast<frame_id_t>(2));  // 验证重复unpin不影响顺序
}

}  // namespace minidb