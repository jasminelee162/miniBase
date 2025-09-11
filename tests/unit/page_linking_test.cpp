// tests/unit/page_linking_test.cpp
#include "storage/storage_engine.h"
#include "storage/page/page_utils.h"
#include "../simple_test_framework.h"
#include <iostream>
#include <cstring>
#include <vector>
#include "../../src/util/config.h"

namespace minidb {

void testPageLinkingBasic() {
    std::cout << "\n=== Test: Page linking basics ===" << std::endl;
    
    // 创建测试数据库
    StorageEngine engine("data/test_page_linking.bin", 64);
    
    // 1. 创建三个页
    page_id_t page1_id, page2_id, page3_id;
    Page* page1 = engine.CreatePage(&page1_id);
    Page* page2 = engine.CreatePage(&page2_id);
    Page* page3 = engine.CreatePage(&page3_id);
    
    ASSERT_NOT_NULL(page1);
    ASSERT_NOT_NULL(page2);
    ASSERT_NOT_NULL(page3);
    
    std::cout << "Created pages: " << page1_id << ", " << page2_id << ", " << page3_id << std::endl;
    
    // 2. 初始化页
    page1->InitializePage(PageType::DATA_PAGE);
    page2->InitializePage(PageType::DATA_PAGE);
    page3->InitializePage(PageType::DATA_PAGE);
    
    // 3. 验证初始状态
    ASSERT_EQ(page1->GetNextPageId(), INVALID_PAGE_ID);
    ASSERT_EQ(page2->GetNextPageId(), INVALID_PAGE_ID);
    ASSERT_EQ(page3->GetNextPageId(), INVALID_PAGE_ID);
    
    // 4. 建立页链接：page1 -> page2 -> page3
    ASSERT_TRUE(engine.LinkPages(page1_id, page2_id));
    ASSERT_TRUE(engine.LinkPages(page2_id, page3_id));
    
    // 5. 验证链接关系
    ASSERT_EQ(page1->GetNextPageId(), page2_id);
    ASSERT_EQ(page2->GetNextPageId(), page3_id);
    ASSERT_EQ(page3->GetNextPageId(), INVALID_PAGE_ID);
    
    std::cout << "Linked: " << page1_id << " -> " << page2_id << " -> " << page3_id << std::endl;
    
    // 6. 归还页
    engine.PutPage(page1_id, true);
    engine.PutPage(page2_id, true);
    engine.PutPage(page3_id, true);
    
    std::cout << "Basic page linking passed!" << std::endl;
}

void testPageChainTraversal() {
    std::cout << "\n=== Test: Page chain traversal ===" << std::endl;
    
    StorageEngine engine("data/test_page_chain.bin", 64);
    
    // 1. 创建5个页并建立链式关系
    std::vector<page_id_t> page_ids;
    std::vector<Page*> pages;
    
    for (int i = 0; i < 5; ++i) {
        page_id_t page_id;
        Page* page = engine.CreatePage(&page_id);
        ASSERT_NOT_NULL(page);
        
        page->InitializePage(PageType::DATA_PAGE);
        page_ids.push_back(page_id);
        pages.push_back(page);
        
        // 建立链接
        if (i > 0) {
            ASSERT_TRUE(engine.LinkPages(page_ids[i-1], page_id));
        }
    }
    
    std::cout << "Created chain: ";
    for (size_t i = 0; i < page_ids.size(); ++i) {
        std::cout << page_ids[i];
        if (i < page_ids.size() - 1) std::cout << " -> ";
    }
    std::cout << std::endl;
    
    // 2. 归还所有页
    for (auto* page : pages) {
        engine.PutPage(page->GetPageId(), true);
    }
    
    // 3. 使用GetPageChain遍历页链
    auto chain_pages = engine.GetPageChain(page_ids[0]);
    ASSERT_EQ(chain_pages.size(), 5);
    
    std::cout << "Traversed chain: ";
    for (size_t i = 0; i < chain_pages.size(); ++i) {
        std::cout << chain_pages[i]->GetPageId();
        if (i < chain_pages.size() - 1) std::cout << " -> ";
    }
    std::cout << std::endl;
    
    // 4. 验证页链顺序
    for (size_t i = 0; i < chain_pages.size(); ++i) {
        ASSERT_EQ(chain_pages[i]->GetPageId(), page_ids[i]);
    }
    
    // 5. 归还遍历得到的页
    for (auto* page : chain_pages) {
        engine.PutPage(page->GetPageId(), false);
    }
    
    std::cout << "Page chain traversal passed!" << std::endl;
}

void testPageLinkingWithData() {
    std::cout << "\n=== Test: Page linking with data ===" << std::endl;
    
    StorageEngine engine("data/test_page_data.bin", 64);
    
    // 1. 创建两个页并链接
    page_id_t page1_id, page2_id;
    Page* page1 = engine.CreatePage(&page1_id);
    Page* page2 = engine.CreatePage(&page2_id);
    
    ASSERT_NOT_NULL(page1);
    ASSERT_NOT_NULL(page2);
    
    page1->InitializePage(PageType::DATA_PAGE);
    page2->InitializePage(PageType::DATA_PAGE);
    
    // 2. 建立链接
    ASSERT_TRUE(engine.LinkPages(page1_id, page2_id));
    
    // 3. 在第一页添加数据
    const char* data1 = "Hello from page 1";
    ASSERT_TRUE(engine.AppendRecordToPage(page1, data1, strlen(data1)));
    
    // 4. 在第二页添加数据
    const char* data2 = "Hello from page 2";
    ASSERT_TRUE(engine.AppendRecordToPage(page2, data2, strlen(data2)));
    
    // 5. 归还页
    engine.PutPage(page1_id, true);
    engine.PutPage(page2_id, true);
    
    // 6. 重新获取页并验证数据
    Page* read_page1 = engine.GetPage(page1_id);
    Page* read_page2 = engine.GetPage(page2_id);
    
    ASSERT_NOT_NULL(read_page1);
    ASSERT_NOT_NULL(read_page2);
    
    // 7. 验证链接关系仍然存在
    ASSERT_EQ(read_page1->GetNextPageId(), page2_id);
    ASSERT_EQ(read_page2->GetNextPageId(), INVALID_PAGE_ID);
    
    // 8. 验证数据
    auto records1 = engine.GetPageRecords(read_page1);
    auto records2 = engine.GetPageRecords(read_page2);
    
    ASSERT_EQ(records1.size(), 1);
    ASSERT_EQ(records2.size(), 1);
    
    std::string record1_str(static_cast<const char*>(records1[0].first), records1[0].second);
    std::string record2_str(static_cast<const char*>(records2[0].first), records2[0].second);
    
    ASSERT_TRUE(record1_str == "Hello from page 1");
    ASSERT_TRUE(record2_str == "Hello from page 2");
    
    std::cout << "Page1 data: " << record1_str << std::endl;
    std::cout << "Page2 data: " << record2_str << std::endl;
    
    // 9. 归还页
    engine.PutPage(page1_id, false);
    engine.PutPage(page2_id, false);
    
    std::cout << "Page linking with data passed!" << std::endl;
}

void testPageLinkingPersistence() {
    std::cout << "\n=== Test: Page linking persistence ===" << std::endl;
    
    // 第一阶段：创建页链并写入数据
    {
        StorageEngine engine("data/test_persistence.bin", 64);
        
        // 创建3个页
        page_id_t page1_id, page2_id, page3_id;
        Page* page1 = engine.CreatePage(&page1_id);
        Page* page2 = engine.CreatePage(&page2_id);
        Page* page3 = engine.CreatePage(&page3_id);
        
        page1->InitializePage(PageType::DATA_PAGE);
        page2->InitializePage(PageType::DATA_PAGE);
        page3->InitializePage(PageType::DATA_PAGE);
        
        // 建立链接
        engine.LinkPages(page1_id, page2_id);
        engine.LinkPages(page2_id, page3_id);
        
        // 添加数据
        engine.AppendRecordToPage(page1, "Page1 Data", 10);
        engine.AppendRecordToPage(page2, "Page2 Data", 10);
        engine.AppendRecordToPage(page3, "Page3 Data", 10);
        
        // 强制刷盘
        engine.Checkpoint();
        
        std::cout << "Phase 1: created chain " << page1_id << " -> " << page2_id << " -> " << page3_id << std::endl;
    }
    
    // 第二阶段：重新打开数据库，验证页链仍然存在
    {
        StorageEngine engine("data/test_persistence.bin", 64);
        
        // 重新打开后，DiskManager 的 next_page_id_ 未持久化，此处按 DEFAULT_MAX_PAGES 探测
        page_id_t first_page_id = INVALID_PAGE_ID;
        for (page_id_t i = 0; i < static_cast<page_id_t>(DEFAULT_MAX_PAGES); ++i) {
            Page* page = engine.GetPage(i);
            if (page && page->GetSlotCount() > 0) {
                first_page_id = i;
                engine.PutPage(i, false);
                break;
            }
            if (page) engine.PutPage(i, false);
        }
        
        ASSERT_TRUE(first_page_id != INVALID_PAGE_ID);
        std::cout << "Found first page id: " << first_page_id << std::endl;
        
        // 遍历页链
        auto chain_pages = engine.GetPageChain(first_page_id);
        ASSERT_TRUE(chain_pages.size() > 0);
        
        std::cout << "Persisted chain: ";
        for (size_t i = 0; i < chain_pages.size(); ++i) {
            std::cout << chain_pages[i]->GetPageId();
            if (i < chain_pages.size() - 1) std::cout << " -> ";
        }
        std::cout << std::endl;
        
        // 验证数据
        for (size_t i = 0; i < chain_pages.size(); ++i) {
            auto records = engine.GetPageRecords(chain_pages[i]);
            ASSERT_EQ(records.size(), 1);
            
            std::string data(static_cast<const char*>(records[0].first), records[0].second);
            std::cout << "Page " << chain_pages[i]->GetPageId() << " data: " << data << std::endl;
        }
        
        // 归还所有页
        for (auto* page : chain_pages) {
            engine.PutPage(page->GetPageId(), false);
        }
    }
    
    std::cout << "Page linking persistence passed!" << std::endl;
}

void testPageLinkingErrorHandling() {
    std::cout << "\n=== Test: Page linking error handling ===" << std::endl;
    
    StorageEngine engine("data/test_error_handling.bin", 64);
    
    // 1. 测试链接不存在的页
    ASSERT_FALSE(engine.LinkPages(999, 1000));  // 不存在的页ID
    
    // 2. 创建两个页
    page_id_t page1_id, page2_id;
    Page* page1 = engine.CreatePage(&page1_id);
    Page* page2 = engine.CreatePage(&page2_id);
    
    page1->InitializePage(PageType::DATA_PAGE);
    page2->InitializePage(PageType::DATA_PAGE);
    
    // 3. 测试链接存在的页
    ASSERT_TRUE(engine.LinkPages(page1_id, page2_id));
    
    // 4. 测试链接到不存在的页
    ASSERT_FALSE(engine.LinkPages(page2_id, 999));
    
    // 5. 测试自链接（应该允许，但可能不是好的实践）
    ASSERT_TRUE(engine.LinkPages(page1_id, page1_id));
    
    // 6. 测试遍历不存在的页链
    auto empty_chain = engine.GetPageChain(999);
    ASSERT_EQ(empty_chain.size(), 0);
    
    // 7. 归还页
    engine.PutPage(page1_id, true);
    engine.PutPage(page2_id, true);
    
    std::cout << "Page linking error handling passed!" << std::endl;
}

} // namespace minidb

int main() {
    SimpleTest::TestSuite suite;
    
    suite.addTest("PageLinkingBasic", minidb::testPageLinkingBasic);
    suite.addTest("PageChainTraversal", minidb::testPageChainTraversal);
    suite.addTest("PageLinkingWithData", minidb::testPageLinkingWithData);
    suite.addTest("PageLinkingPersistence", minidb::testPageLinkingPersistence);
    suite.addTest("PageLinkingErrorHandling", minidb::testPageLinkingErrorHandling);
    
    suite.runAll();
    
    return SimpleTest::TestCase::getFailed() > 0 ? 1 : 0;
}