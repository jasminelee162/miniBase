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
        #ifdef PROJECT_ROOT_DIR
        test_db_file_ = std::string(PROJECT_ROOT_DIR) + "/data/test_storage.bin";
        #else
        test_db_file_ = "test_storage.bin";
        #endif
        // std::filesystem::remove(test_db_file_);  //不同用例写入不删除文件
        storage_engine_ = std::make_unique<StorageEngine>(test_db_file_, 64);  //调用diskmanager构造函数，缓冲池大小64页
    }
    
    // void TearDown() override {
    //     storage_engine_.reset();
    //     // std::filesystem::remove(test_db_file_);
    // }
    
    std::string test_db_file_;
    std::unique_ptr<StorageEngine> storage_engine_;
};
//测试一：写入Hello MiniDB Storage!字符串，标记脏并归还，读出来比对内容，归还
TEST_F(StorageEngineTest, BasicPageOperations) {
    // 创建页面
    page_id_t page_id;
    Page* page = storage_engine_->CreatePage(&page_id);
    ASSERT_NE(page, nullptr);
    
    // 写入数据
    const char* test_data = "Hello MiniDB Storage Niceeeeeee!";
    std::memcpy(page->GetData(), test_data, strlen(test_data));
    EXPECT_TRUE(storage_engine_->PutPage(page_id, true));
    
    // 读取数据
    Page* read_page = storage_engine_->GetPage(page_id);
    ASSERT_NE(read_page, nullptr);
    EXPECT_EQ(std::memcmp(read_page->GetData(), test_data, strlen(test_data)), 0);
    storage_engine_->PutPage(page_id);
}
// //测试二：4个线程，每个线程并发写入10个页，测试并发写入。最后检查总共成功创建了40页
// TEST_F(StorageEngineTest, ConcurrentAccess) {
//     const int num_threads = 4;
//     const int pages_per_thread = 10;
//     std::vector<std::thread> threads;
//     std::atomic<int> success_count{0};
    
//     for (int t = 0; t < num_threads; ++t) {
//         threads.emplace_back([&, t]() {
//             for (int i = 0; i < pages_per_thread; ++i) {
//                 page_id_t page_id;
//                 Page* page = storage_engine_->CreatePage(&page_id);
//                 if (page != nullptr) {
//                     // 写入线程特定数据
//                     int data = t * 1000 + i;
//                     std::memcpy(page->GetData(), &data, sizeof(data));
//                     storage_engine_->PutPage(page_id, true);
//                     success_count.fetch_add(1);
//                 }
//             }
//         });
//     }
    
//     for (auto& thread : threads) {
//         thread.join();
//     }
    
//     const size_t want_total = static_cast<size_t>(num_threads * pages_per_thread);
//     const size_t expect_total = std::min(want_total, DEFAULT_MAX_PAGES);
//     EXPECT_EQ(static_cast<size_t>(success_count.load()), expect_total);
// }
// //测试三：测试缓存效果，当前缓存容量64，创建100个页，重复访问前50个页，访问三轮，测试缓存命中率,并打印统计信息（缓冲池大小、命中率、淘汰次数、写回次数）。
// TEST_F(StorageEngineTest, CachePerformance) {
//     // 创建足够多的页面以触发替换（不超过容量上限）
//     const size_t want_pages = 100;
//     const size_t create_pages = std::min(want_pages, DEFAULT_MAX_PAGES);
//     std::vector<page_id_t> page_ids;
//     page_ids.reserve(create_pages);
//     for (size_t i = 0; i < create_pages; ++i) {
//         page_id_t page_id;
//         Page* page = storage_engine_->CreatePage(&page_id);
//         ASSERT_NE(page, nullptr);
//         page_ids.push_back(page_id);
//         storage_engine_->PutPage(page_id);
//     }
    
//     // 重复访问前50个页面
//     const size_t reuse_count = std::min<size_t>(50, page_ids.size());
//     for (int round = 0; round < 3; ++round) {
//         for (size_t i = 0; i < reuse_count; ++i) {
//             Page* page = storage_engine_->GetPage(page_ids[i]);
//             EXPECT_NE(page, nullptr);
//             storage_engine_->PutPage(page_ids[i]);
//         }
//     }
    
//     double hit_rate = storage_engine_->GetCacheHitRate();
//     EXPECT_GT(hit_rate, 0.3);  // 缓存命中率应该大于30%
    
//     storage_engine_->PrintStats();
// }
// //测试四：测试LRU替换策略，创建一个LRU替换器，插入1、2、3，验证淘汰顺序为1，然后插入2、3，验证淘汰顺序为3，最后验证重复unpin不影响顺序。
// TEST_F(StorageEngineTest, LRUReplacementPolicy) {
//     LRUReplacer replacer(2);

//     replacer.Unpin(1);
//     replacer.Unpin(2);
//     replacer.Unpin(3);
    
//     frame_id_t victim;
//     EXPECT_TRUE(replacer.Victim(&victim));
//     EXPECT_EQ(victim, static_cast<frame_id_t>(1));  // 修正预期淘汰顺序
    
//     replacer.Pin(2);
//     replacer.Unpin(3);
//     EXPECT_FALSE(replacer.Victim(&victim));
    
//     replacer.Unpin(2);
//     EXPECT_TRUE(replacer.Victim(&victim));
//     EXPECT_EQ(victim, static_cast<frame_id_t>(3));
    
//     // 新增测试重复unpin场景
//     replacer.Unpin(1);
//     replacer.Unpin(2);
//     replacer.Unpin(1);
//     replacer.Unpin(3);
    
//     EXPECT_TRUE(replacer.Victim(&victim));
//     EXPECT_EQ(victim, static_cast<frame_id_t>(2));  // 验证重复unpin不影响顺序
// }

// // // 追加一个用例：确保生成并保留测试数据库文件，便于外部检查
// // TEST_F(StorageEngineTest, PersistFileForInspection) {
// //     // 打印当前工作目录与目标文件的绝对路径
// //     std::cout << "CWD=" << std::filesystem::current_path() << std::endl;
// //     std::cout << "DB=" << std::filesystem::absolute(test_db_file_) << std::endl;

// //     // 创建一页并标脏，确保有内容写入
// //     page_id_t page_id;
// //     Page* page = storage_engine_->CreatePage(&page_id);
// //     ASSERT_NE(page, nullptr);
// //     const char* msg = "Persist check";
// //     std::memcpy(page->GetData(), msg, std::strlen(msg));
// //     EXPECT_TRUE(storage_engine_->PutPage(page_id, true));

// //     // 强制刷盘，确保文件可见
// //     storage_engine_->Checkpoint();
// //     // 不删除文件，测试结束后应在工作目录下可见
// // }

// // TEST_F(StorageEngineTest, CreatesDbFileWithDefaultSize) {
// //     // 验证数据库文件被创建
// //     ASSERT_TRUE(std::filesystem::exists(test_db_file_));

// //     // 验证文件大小等于 DEFAULT_DISK_SIZE_BYTES（新建时预分配）
// //     auto size = std::filesystem::file_size(test_db_file_);
// //     EXPECT_EQ(size, DEFAULT_DISK_SIZE_BYTES);
// // }

 }  // namespace minidb