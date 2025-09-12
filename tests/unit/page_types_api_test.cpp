// tests/unit/page_types_api_test.cpp
#include "storage/storage_engine.h"
#include "../simple_test_framework.h"
#include <cstring>
#include <string>

using namespace minidb;

static std::string make_db_path(const char* fname) {
#ifdef PROJECT_ROOT_DIR
    return std::string(PROJECT_ROOT_DIR) + "/data/" + fname;
#else
    return std::string(fname);
#endif
}

void test_meta_and_catalog_pages() {
    std::string db = make_db_path("test_page_types_api.bin");
    StorageEngine engine(db, 64);

    // Meta init and get/set
    ASSERT_TRUE(engine.InitializeMetaPage());
    MetaInfo m = engine.GetMetaInfo();
    ASSERT_EQ((uint64_t)0x4D696E6944425F4DULL, m.magic);
    ASSERT_EQ((uint32_t)1, m.version);
    ASSERT_EQ((uint32_t)PAGE_SIZE, m.page_size);
    ASSERT_EQ((page_id_t)1, m.next_page_id);
    ASSERT_EQ((page_id_t)INVALID_PAGE_ID, m.catalog_root);

    // Update and verify
    m.next_page_id = 10;
    ASSERT_TRUE(engine.UpdateMetaInfo(m));
    ASSERT_EQ((page_id_t)10, engine.GetNextPageId());

    // Create catalog page and verify root/roundtrip
    Page* cat = engine.CreateCatalogPage();
    ASSERT_NOT_NULL(cat);
    ASSERT_EQ((uint32_t)PageType::CATALOG_PAGE, cat->GetHeader()->page_type);
    ASSERT_NE((page_id_t)INVALID_PAGE_ID, engine.GetCatalogRoot());

    // Write some catalog bytes
    std::vector<char> bytes = {'M','e','t','a'};
    ASSERT_TRUE(engine.UpdateCatalogData(CatalogData(bytes)));

    engine.PutPage(cat->GetPageId(), false);
    engine.Shutdown();
}

void test_data_pages_basic_io_and_link() {
    std::string db = make_db_path("test_page_types_api.bin");
    StorageEngine engine(db, 64);

    // Ensure meta exists
    engine.InitializeMetaPage();

    // Create first data page
    page_id_t p1 = INVALID_PAGE_ID;
    Page* d1 = engine.CreateDataPage(&p1);
    ASSERT_NOT_NULL(d1);
    ASSERT_EQ((uint32_t)PageType::DATA_PAGE, d1->GetHeader()->page_type);

    // Append a few records
    const char* r1 = "hello";
    const char* r2 = "world";
    ASSERT_TRUE(engine.AppendRecordToPage(d1, r1, (uint16_t)std::strlen(r1)));
    ASSERT_TRUE(engine.AppendRecordToPage(d1, r2, (uint16_t)std::strlen(r2)));

    auto recs1 = engine.GetPageRecords(d1);
    ASSERT_EQ((size_t)2, recs1.size());

    // Create and link a second data page
    page_id_t p2 = INVALID_PAGE_ID;
    Page* d2 = engine.CreateDataPage(&p2);
    ASSERT_NOT_NULL(d2);
    ASSERT_TRUE(engine.LinkPages(p1, p2));

    // Chain traversal should return two pages
    auto chain = engine.GetPageChain(p1);
    ASSERT_EQ((size_t)2, chain.size());

    engine.PutPage(p1, true);
    engine.PutPage(p2, true);
    engine.Shutdown();
}

void test_index_pages_create_and_type_check() {
    std::string db = make_db_path("test_page_types_api.bin");
    StorageEngine engine(db, 64);
    engine.InitializeMetaPage();

    page_id_t ipid = INVALID_PAGE_ID;
    Page* idx = engine.CreateIndexPage(&ipid);
    ASSERT_NOT_NULL(idx);
    ASSERT_EQ((uint32_t)PageType::INDEX_PAGE, idx->GetHeader()->page_type);

    // Fetch by GetIndexPage and verify type
    Page* idx2 = engine.GetIndexPage(ipid);
    ASSERT_NOT_NULL(idx2);
    ASSERT_EQ((uint32_t)PageType::INDEX_PAGE, idx2->GetHeader()->page_type);

    engine.PutPage(ipid, true);
    engine.Shutdown();
}

int main() {
    SimpleTest::TestSuite suite;
    suite.addTest("MetaAndCatalogPages", test_meta_and_catalog_pages);
    suite.addTest("DataPagesBasicIOAndLink", test_data_pages_basic_io_and_link);
    suite.addTest("IndexPagesCreateAndTypeCheck", test_index_pages_create_and_type_check);
    suite.runAll();
    return SimpleTest::TestCase::getFailed() > 0 ? 1 : 0;
}


