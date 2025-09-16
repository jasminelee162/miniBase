#include "../simple_test_framework.h"
#include "../../src/storage/storage_engine.h"
#include "../../src/storage/page/page.h"

using namespace minidb;
using namespace SimpleTest;

int main() {
    TestSuite suite;

    suite.addTest("data/index page type validation", [](){
        StorageEngine se("data/test_page_type_validation.db", 32);
        // Create data page
        page_id_t pid = INVALID_PAGE_ID;
        Page* p = se.CreateDataPage(&pid);
        ASSERT_NOT_NULL(p);
        ASSERT_NE(INVALID_PAGE_ID, pid);
        ASSERT_EQ(static_cast<size_t>(PageType::DATA_PAGE), static_cast<size_t>(p->GetPageType()));
        se.PutPage(pid, false);

        // GetDataPage should succeed; GetIndexPage should reject
        Page* dp = se.GetDataPage(pid);
        ASSERT_NOT_NULL(dp);
        se.PutPage(pid, false);
        Page* ip = se.GetIndexPage(pid);
        ASSERT_TRUE(ip == nullptr);
    });

    suite.addTest("index page type validation", [](){
        StorageEngine se("data/test_page_type_validation.db", 32);
        page_id_t pid = INVALID_PAGE_ID;
        Page* p = se.CreateIndexPage(&pid);
        ASSERT_NOT_NULL(p);
        ASSERT_NE(INVALID_PAGE_ID, pid);
        ASSERT_EQ(static_cast<size_t>(PageType::INDEX_PAGE), static_cast<size_t>(p->GetPageType()));
        se.PutPage(pid, false);

        Page* ip = se.GetIndexPage(pid);
        ASSERT_NOT_NULL(ip);
        se.PutPage(pid, false);
        Page* dp = se.GetDataPage(pid);
        ASSERT_TRUE(dp == nullptr);
    });

    suite.runAll();
    return TestCase::getFailed();
}


