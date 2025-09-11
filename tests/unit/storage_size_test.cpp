// tests/unit/storage_test.cpp
#include "storage/storage_engine.h"
#include "../simple_test_framework.h"
#include <filesystem>
#include <thread>
#include <vector>

namespace minidb {

class StorageEngineTest {
public:
    void setUp() {
        #ifdef PROJECT_ROOT_DIR
        test_db_file_ = std::string(PROJECT_ROOT_DIR) + "/data/test_size.bin";
        #else
        test_db_file_ = "test_size.bin";
        #endif
        // std::filesystem::remove(test_db_file_);  //不同用例写入不删除文件
        storage_engine_ = std::make_unique<StorageEngine>(test_db_file_, 64);  //调用diskmanager构造函数，缓冲池大小64页
    }
    
    void tearDown() {
        storage_engine_.reset();
        // std::filesystem::remove(test_db_file_);
    }
    
    std::string test_db_file_;
    std::unique_ptr<StorageEngine> storage_engine_;
};

//测试一：写入Hello MiniDB Storage!字符串，标记脏并归还，读出来比对内容，归还
void testBasicPageOperations() {
    StorageEngineTest test;
    test.setUp();
    
    // 创建页面
    page_id_t page_id;
    Page* page = test.storage_engine_->CreatePage(&page_id);
    ASSERT_NOT_NULL(page);
    
    // 写入数据
    const char* test_data = "Hello MiniDB aaabbb Storage Niceee!";
    std::memcpy(page->GetData(), test_data, strlen(test_data));
    EXPECT_TRUE(test.storage_engine_->PutPage(page_id, true));
    
    // 读取数据
    Page* read_page = test.storage_engine_->GetPage(page_id);
    ASSERT_NOT_NULL(read_page);
    EXPECT_EQ(std::memcmp(read_page->GetData(), test_data, strlen(test_data)), 0);
    test.storage_engine_->PutPage(page_id);
    
    // 打印统计信息，展示新的logger功能
    test.storage_engine_->PrintStats();
    
    test.tearDown();
}
}  // namespace minidb

int main() {
    SimpleTest::TestSuite suite;
    
    suite.addTest("BasicPageOperations", minidb::testBasicPageOperations);
    
    suite.runAll();
    
    return SimpleTest::TestCase::getFailed() > 0 ? 1 : 0;
}