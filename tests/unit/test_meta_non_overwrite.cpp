#include "../simple_test_framework.h"
#include "../../src/storage/storage_engine.h"
#include "../../src/storage/page/disk_manager.h"

using namespace minidb;
using namespace SimpleTest;

int main(){
    TestSuite suite;

    suite.addTest("meta non-overwrite preserves catalog_root", [](){
        StorageEngine se("data/test_meta_non_overwrite.db", 32);
        // Create and set a catalog root
        Page* cat = se.CreateCatalogPage();
        ASSERT_NOT_NULL(cat);
        page_id_t root = cat->GetPageId();
        se.PutPage(root, true);

        // Verify
        ASSERT_EQ(root, se.GetCatalogRoot());

        // Persist meta
        se.Checkpoint();

        // Reopen engine and check the same catalog_root
        se.Shutdown();
        StorageEngine se2("data/test_meta_non_overwrite.db", 32);
        ASSERT_EQ(root, se2.GetCatalogRoot());
    });

    suite.runAll();
    return TestCase::getFailed();
}


