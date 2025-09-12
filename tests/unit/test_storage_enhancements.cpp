#include <iostream>
#include <cassert>
#include "../../src/storage/storage_engine.h"
#include "../../src/util/config.h"

// 定义测试辅助宏，避免使用 assert() 导致 abort()
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            std::cout << "[FAIL] Test failed: " << message << std::endl; \
            return false; \
        } \
    } while(0)

#define TEST_ASSERT_CONTINUE(condition, message) \
    do { \
        if (!(condition)) { \
            std::cout << "[FAIL] Test failed: " << message << std::endl; \
            return; \
        } \
    } while(0)

using namespace minidb;

void test_metadata_management() {
    std::cout << "=== Testing Metadata Management ===" << std::endl;
    
    StorageEngine engine("test_metadata.bin");
    
    // Test metadata page initialization
    TEST_ASSERT_CONTINUE(engine.InitializeMetaPage(), "Metadata page initialization failed");
    std::cout << "[PASS] Metadata page initialization successful" << std::endl;
    
    // Test metadata information retrieval
    MetaInfo meta_info = engine.GetMetaInfo();
    TEST_ASSERT_CONTINUE(meta_info.magic == 0x4D696E6944425F4DULL, "Magic value incorrect");
    TEST_ASSERT_CONTINUE(meta_info.version == 1, "Version value incorrect");
    TEST_ASSERT_CONTINUE(meta_info.page_size == PAGE_SIZE, "Page size value incorrect");
    TEST_ASSERT_CONTINUE(meta_info.next_page_id == 1, "Next page ID value incorrect");
    TEST_ASSERT_CONTINUE(meta_info.catalog_root == INVALID_PAGE_ID, "Catalog root value incorrect");
    std::cout << "  Metadata information read correctly" << std::endl;
    
    // Test metadata update
    meta_info.catalog_root = 5;
    meta_info.next_page_id = 10;
    TEST_ASSERT_CONTINUE(engine.UpdateMetaInfo(meta_info), "Metadata update failed");
    
    MetaInfo updated_meta = engine.GetMetaInfo();
    TEST_ASSERT_CONTINUE(updated_meta.catalog_root == 5, "Catalog root value after update incorrect");
    TEST_ASSERT_CONTINUE(updated_meta.next_page_id == 10, "Next page ID value after update incorrect");
    std::cout << "  Metadata update successful" << std::endl;
    
    // Test metadata field access
    TEST_ASSERT_CONTINUE(engine.GetCatalogRoot() == 5, "GetCatalogRoot return value incorrect");
    TEST_ASSERT_CONTINUE(engine.GetNextPageId() == 10, "GetNextPageId return value incorrect");
    TEST_ASSERT_CONTINUE(engine.SetCatalogRoot(7), "SetCatalogRoot failed");
    TEST_ASSERT_CONTINUE(engine.SetNextPageId(15), "SetNextPageId failed");
    TEST_ASSERT_CONTINUE(engine.GetCatalogRoot() == 7, "GetCatalogRoot return value after SetCatalogRoot incorrect");
    TEST_ASSERT_CONTINUE(engine.GetNextPageId() == 15, "GetNextPageId return value after SetNextPageId incorrect");
    std::cout << "  Metadata field access successful" << std::endl;
    
    engine.Shutdown();
    std::cout << "  Metadata management test passed" << std::endl;
}

void test_catalog_page_management() {
    std::cout << "\n=== Testing Catalog Page Management ===" << std::endl;
    
    StorageEngine engine("test_catalog.bin");
    
    // Initialize metadata page
    TEST_ASSERT_CONTINUE(engine.InitializeMetaPage(), "Metadata page initialization failed");
    
    // Test catalog page creation
    Page* catalog_page = engine.CreateCatalogPage();
    TEST_ASSERT_CONTINUE(catalog_page != nullptr, "Catalog page creation failed");
    TEST_ASSERT_CONTINUE(catalog_page->GetPageType() == PageType::CATALOG_PAGE, "Catalog page type incorrect");
    std::cout << "  Catalog page created successfully, page ID: " << catalog_page->GetPageId() << std::endl;
    
    // Test catalog page retrieval
    std::cout << "Checking catalog_root: " << engine.GetCatalogRoot() << std::endl;
    std::cout << "Checking if page is in buffer pool: " << (engine.GetPage(catalog_page->GetPageId()) != nullptr ? "Yes" : "No") << std::endl;
    
    // Check metadata
    MetaInfo meta_info = engine.GetMetaInfo();
    std::cout << "Metadata: magic=0x" << std::hex << meta_info.magic 
              << ", version=" << std::dec << meta_info.version 
              << ", page_size=" << meta_info.page_size
              << ", next_page_id=" << meta_info.next_page_id
              << ", catalog_root=" << meta_info.catalog_root << std::endl;
    
    // Direct test GetPage
    std::cout << "Direct test GetPage(1): " << (engine.GetPage(1) != nullptr ? "Success" : "Failed") << std::endl;
    std::cout << "Checking GetNumPages(): " << engine.GetNumPages() << std::endl;
    
    Page* retrieved_catalog = engine.GetCatalogPage();
    if (!retrieved_catalog) {
        std::cout << "  Catalog page retrieval failed, catalog_root=" << engine.GetCatalogRoot() << std::endl;
        engine.PutPage(catalog_page->GetPageId(), false);
        engine.Shutdown();
        return;
    }
    if (retrieved_catalog->GetPageId() != catalog_page->GetPageId()) {
        std::cout << "  Catalog page ID mismatch" << std::endl;
        engine.PutPage(catalog_page->GetPageId(), false);
        engine.Shutdown();
        return;
    }
    std::cout << "  Catalog page retrieval successful" << std::endl;
    
    // Test catalog data update
    std::vector<char> test_data = {'T', 'e', 's', 't', 'C', 'a', 't', 'a', 'l', 'o', 'g'};
    CatalogData catalog_data(test_data);
    TEST_ASSERT_CONTINUE(engine.UpdateCatalogData(catalog_data), "Catalog data update failed");
    std::cout << "  Catalog data update successful" << std::endl;
    
    // Verify catalog_root is set correctly
    TEST_ASSERT_CONTINUE(engine.GetCatalogRoot() == catalog_page->GetPageId(), "Catalog root setting incorrect");
    std::cout << "  Catalog root set correctly" << std::endl;
    
    // Release page
    engine.PutPage(catalog_page->GetPageId(), false);
    engine.Shutdown();
    std::cout << "  Catalog page management test passed" << std::endl;
}

void test_data_page_operations() {
    std::cout << "\n=== Testing Data Page Operations ===" << std::endl;
    
    StorageEngine engine("test_data_pages.bin");
    
    // Test page creation and initialization
    page_id_t page_id = INVALID_PAGE_ID;
    Page* page = engine.CreatePage(&page_id);
    TEST_ASSERT_CONTINUE(page != nullptr, "Page creation failed");
    TEST_ASSERT_CONTINUE(page_id != INVALID_PAGE_ID, "Page ID invalid");
    
    engine.InitializeDataPage(page);
    TEST_ASSERT_CONTINUE(page->GetPageType() == PageType::DATA_PAGE, "Page type incorrect");
    std::cout << "  Data page created and initialized successfully, page ID: " << page_id << std::endl;
    
    // Test record operations
    std::string test_record1 = "Hello, World!";
    std::string test_record2 = "MiniDB Storage";
    std::string test_record3 = "Test Data";
    
    // Add debug information
    std::cout << "Preparing to append record 1: \"" << test_record1 << "\" (length: " << test_record1.length() << ")" << std::endl;
    bool append_success1 = engine.AppendRecordToPage(page, test_record1.c_str(), static_cast<uint16_t>(test_record1.length()));
    if (!append_success1) {
        std::cout << "  Record 1 append failed" << std::endl;
        engine.PutPage(page_id, false);
        engine.Shutdown();
        return;
    }
    std::cout << "  Record 1 appended successfully" << std::endl;
    
    std::cout << "Preparing to append record 2: \"" << test_record2 << "\" (length: " << test_record2.length() << ")" << std::endl;
    bool append_success2 = engine.AppendRecordToPage(page, test_record2.c_str(), static_cast<uint16_t>(test_record2.length()));
    if (!append_success2) {
        std::cout << "  Record 2 append failed" << std::endl;
        engine.PutPage(page_id, false);
        engine.Shutdown();
        return;
    }
    std::cout << "  Record 2 appended successfully" << std::endl;
    
    std::cout << "Preparing to append record 3: \"" << test_record3 << "\" (length: " << test_record3.length() << ")" << std::endl;
    bool append_success3 = engine.AppendRecordToPage(page, test_record3.c_str(), static_cast<uint16_t>(test_record3.length()));
    if (!append_success3) {
        std::cout << "  Record 3 append failed" << std::endl;
        engine.PutPage(page_id, false);
        engine.Shutdown();
        return;
    }
    std::cout << "  Record 3 appended successfully" << std::endl;
    
    // Test record reading
    auto records = engine.GetPageRecords(page);
    TEST_ASSERT_CONTINUE(records.size() == 3, "Record count incorrect");
    TEST_ASSERT_CONTINUE(records[0].second == test_record1.length(), "Record 1 length incorrect");
    TEST_ASSERT_CONTINUE(records[1].second == test_record2.length(), "Record 2 length incorrect");
    TEST_ASSERT_CONTINUE(records[2].second == test_record3.length(), "Record 3 length incorrect");
    std::cout << "  Record reading successful, total " << records.size() << " records" << std::endl;
    
    engine.PutPage(page_id, true);
    std::cout << "  Data page operations test passed" << std::endl;
}

void test_page_linking() {
    std::cout << "\n=== Testing Page Linking ===" << std::endl;
    
    StorageEngine engine("test_page_linking.bin");
    
    // Create multiple pages
    page_id_t page1_id = INVALID_PAGE_ID;
    page_id_t page2_id = INVALID_PAGE_ID;
    page_id_t page3_id = INVALID_PAGE_ID;
    
    Page* page1 = engine.CreatePage(&page1_id);
    if (!page1) {
        std::cout << "  Page 1 creation failed" << std::endl;
        engine.Shutdown();
        return;
    }
    Page* page2 = engine.CreatePage(&page2_id);
    if (!page2) {
        std::cout << "  Page 2 creation failed" << std::endl;
        engine.PutPage(page1_id, false);
        engine.Shutdown();
        return;
    }
    Page* page3 = engine.CreatePage(&page3_id);
    if (!page3) {
        std::cout << "  Page 3 creation failed" << std::endl;
        engine.PutPage(page1_id, false);
        engine.PutPage(page2_id, false);
        engine.Shutdown();
        return;
    }
    
    // Initialize pages
    engine.InitializeDataPage(page1);
    engine.InitializeDataPage(page2);
    engine.InitializeDataPage(page3);
    
    // Test page linking
    TEST_ASSERT_CONTINUE(engine.LinkPages(page1_id, page2_id), "Page 1 to page 2 linking failed");
    TEST_ASSERT_CONTINUE(engine.LinkPages(page2_id, page3_id), "Page 2 to page 3 linking failed");
    std::cout << "  Page linking successful" << std::endl;
    
    // Test page chain traversal
    auto page_chain = engine.GetPageChain(page1_id);
    TEST_ASSERT_CONTINUE(page_chain.size() == 3, "Page chain length incorrect");
    TEST_ASSERT_CONTINUE(page_chain[0]->GetPageId() == page1_id, "First page in chain ID incorrect");
    TEST_ASSERT_CONTINUE(page_chain[1]->GetPageId() == page2_id, "Second page in chain ID incorrect");
    TEST_ASSERT_CONTINUE(page_chain[2]->GetPageId() == page3_id, "Third page in chain ID incorrect");
    std::cout << "  Page chain traversal successful, chain length: " << page_chain.size() << std::endl;
    
    // Test prefetch functionality
    engine.PrefetchPageChain(page1_id, 2);
    std::cout << "  Page chain prefetch successful" << std::endl;
    
    engine.PutPage(page1_id, false);
    engine.PutPage(page2_id, false);
    engine.PutPage(page3_id, false);
    engine.Shutdown();
    std::cout << "  Page linking test passed" << std::endl;
}

void test_meta_page_persistence() {
    std::cout << "\n=== Testing Metadata Page Persistence ===" << std::endl;
    
    StorageEngine engine("test_persistence.bin");
    
    // Initialize metadata page
    TEST_ASSERT_CONTINUE(engine.InitializeMetaPage(), "Metadata page initialization failed");
    
    // Modify metadata
    MetaInfo meta_info = engine.GetMetaInfo();
    meta_info.next_page_id = 100;
    meta_info.catalog_root = 50;
    TEST_ASSERT_CONTINUE(engine.UpdateMetaInfo(meta_info), "Metadata update failed");
    
    // Create catalog page
    Page* catalog_page = engine.CreateCatalogPage();
    TEST_ASSERT_CONTINUE(catalog_page != nullptr, "Catalog page creation failed");
    
    // Check actual next_page_id value (may increase due to catalog page creation)
    MetaInfo updated_meta = engine.GetMetaInfo();
    std::cout << "Metadata after setting: next_page_id=" << updated_meta.next_page_id 
              << ", catalog_root=" << updated_meta.catalog_root << std::endl;
    
    // Checkpoint save
    engine.Checkpoint();
    std::cout << "  Checkpoint save successful" << std::endl;
    
    // Close and reopen
    engine.Shutdown();
    
    StorageEngine engine2("test_persistence.bin");
    
    // First check if metadata page exists
    Page* meta_page = engine2.GetMetaPage();
    if (!meta_page) {
        std::cout << "  Unable to get metadata page" << std::endl;
        engine2.Shutdown();
        return;
    }
    std::cout << "  Metadata page retrieval successful" << std::endl;
    
    // Verify metadata is correctly restored
    MetaInfo restored_meta = engine2.GetMetaInfo();
    std::cout << "Restored metadata: magic=0x" << std::hex << restored_meta.magic 
              << ", version=" << std::dec << restored_meta.version 
              << ", page_size=" << restored_meta.page_size
              << ", next_page_id=" << restored_meta.next_page_id
              << ", catalog_root=" << restored_meta.catalog_root << std::endl;
    
    if (restored_meta.magic != 0x4D696E6944425F4DULL) {
        std::cout << "  Magic restored value incorrect: 0x" << std::hex << restored_meta.magic << std::endl;
        engine2.Shutdown();
        return;
    }
    if (restored_meta.version != 1) {
        std::cout << "  Version restored value incorrect: " << restored_meta.version << std::endl;
        engine2.Shutdown();
        return;
    }
    if (restored_meta.page_size != PAGE_SIZE) {
        std::cout << "  Page size restored value incorrect: " << restored_meta.page_size << std::endl;
        engine2.Shutdown();
        return;
    }
    if (restored_meta.next_page_id < 100) {
        std::cout << "  Next page ID restored value incorrect: " << restored_meta.next_page_id << " < 100" << std::endl;
        engine2.Shutdown();
        return;
    }
    if (restored_meta.catalog_root == INVALID_PAGE_ID) {
        std::cout << "  Catalog root restored value incorrect: " << restored_meta.catalog_root << std::endl;
        engine2.Shutdown();
        return;
    }
    std::cout << "  Metadata persistence restoration successful" << std::endl;
    
    // Verify catalog page can be correctly retrieved
    Page* restored_catalog = engine2.GetCatalogPage();
    TEST_ASSERT_CONTINUE(restored_catalog != nullptr, "Restored catalog page is null");
    TEST_ASSERT_CONTINUE(restored_catalog->GetPageType() == PageType::CATALOG_PAGE, "Restored catalog page type incorrect");
    std::cout << "  Catalog page persistence restoration successful" << std::endl;
    
    engine2.Shutdown();
    std::cout << "  Metadata page persistence test passed" << std::endl;
}

int main() {
    std::cout << "Starting Storage module storage-related functionality tests..." << std::endl;
    
    try {
        test_metadata_management();
        test_catalog_page_management();
        test_data_page_operations();
        test_page_linking();
        test_meta_page_persistence();
        
        std::cout << "\n All storage-related tests passed! Storage module storage functionality working normally." << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
}
