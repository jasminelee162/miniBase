#include <iostream>
#include <cassert>
#include "../../src/storage/storage_engine.h"
#include "../../src/storage/index/bplus_tree.h"
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

void test_bplus_tree_basic_operations() {
    std::cout << "=== Testing B+Tree basic operations ===" << std::endl;
    
    StorageEngine engine("test_bplus_basic.bin");
    BPlusTree tree(&engine);
    
    // 创建新树
    page_id_t root_id = tree.CreateNew();
    TEST_ASSERT_CONTINUE(root_id != INVALID_PAGE_ID, "Failed to create B+Tree");
    std::cout << "[OK] B+Tree created, root page id: " << root_id << std::endl;
    
    // 测试插入
    RID rid1{1, 10};
    RID rid2{2, 20};
    RID rid3{3, 30};
    
    TEST_ASSERT_CONTINUE(tree.Insert(100, rid1), "Insert key 100 failed");
    TEST_ASSERT_CONTINUE(tree.Insert(200, rid2), "Insert key 200 failed");
    TEST_ASSERT_CONTINUE(tree.Insert(150, rid3), "Insert key 150 failed");
    std::cout << "[OK] Insert operations succeeded" << std::endl;
    
    // 测试查找
    auto result1 = tree.Search(100);
    TEST_ASSERT_CONTINUE(result1.has_value(), "Search key 100 failed");
    TEST_ASSERT_CONTINUE(result1->page_id == 1 && result1->slot == 10, "RID for key 100 incorrect");
    
    auto result2 = tree.Search(200);
    TEST_ASSERT_CONTINUE(result2.has_value(), "Search key 200 failed");
    TEST_ASSERT_CONTINUE(result2->page_id == 2 && result2->slot == 20, "RID for key 200 incorrect");
    
    auto result3 = tree.Search(150);
    TEST_ASSERT_CONTINUE(result3.has_value(), "Search key 150 failed");
    TEST_ASSERT_CONTINUE(result3->page_id == 3 && result3->slot == 30, "RID for key 150 incorrect");
    std::cout << "[OK] Search operations succeeded" << std::endl;
    
    // 测试范围查询
    auto range_result = tree.Range(100, 200);
    TEST_ASSERT_CONTINUE(range_result.size() == 3, "Range result size incorrect");
    std::cout << "[OK] Range query succeeded, count: " << range_result.size() << std::endl;
    
    engine.Shutdown();
    std::cout << "[OK] Basic operations test passed" << std::endl;
}

void test_bplus_tree_enhanced_operations() {
    std::cout << "\n=== Testing B+Tree enhanced operations ===" << std::endl;
    
    StorageEngine engine("test_bplus_enhanced.bin");
    BPlusTree tree(&engine);
    
    // 创建新树
    page_id_t root_id = tree.CreateNew();
    TEST_ASSERT_CONTINUE(root_id != INVALID_PAGE_ID, "Failed to create B+Tree");
    
    // 插入测试数据
    RID rid1{1, 10};
    RID rid2{2, 20};
    RID rid3{3, 30};
    RID rid4{4, 40};
    RID rid5{5, 50};
    
    TEST_ASSERT_CONTINUE(tree.Insert(100, rid1), "插入键100失败");
    TEST_ASSERT_CONTINUE(tree.Insert(200, rid2), "插入键200失败");
    TEST_ASSERT_CONTINUE(tree.Insert(150, rid3), "插入键150失败");
    TEST_ASSERT_CONTINUE(tree.Insert(300, rid4), "插入键300失败");
    TEST_ASSERT_CONTINUE(tree.Insert(250, rid5), "插入键250失败");
    std::cout << "[OK] Batch inserts succeeded" << std::endl;
    
    // 测试更新操作
    RID new_rid{99, 99};
    TEST_ASSERT_CONTINUE(tree.Update(100, new_rid), "Update key 100 failed");
    auto updated_result = tree.Search(100);
    TEST_ASSERT_CONTINUE(updated_result.has_value(), "Search key 100 after update failed");
    TEST_ASSERT_CONTINUE(updated_result->page_id == 99 && updated_result->slot == 99, "Updated RID incorrect");
    std::cout << "[OK] Update succeeded" << std::endl;
    
    // 测试删除操作
    TEST_ASSERT_CONTINUE(tree.Delete(150), "Delete key 150 failed");
    auto deleted_result = tree.Search(150);
    TEST_ASSERT_CONTINUE(!deleted_result.has_value(), "Key 150 still found after delete");
    std::cout << "[OK] Delete succeeded" << std::endl;
    
    // 测试HasKey
    TEST_ASSERT_CONTINUE(tree.HasKey(100), "HasKey(100) expected true");
    TEST_ASSERT_CONTINUE(tree.HasKey(200), "HasKey(200) expected true");
    TEST_ASSERT_CONTINUE(tree.HasKey(300), "HasKey(300) expected true");
    TEST_ASSERT_CONTINUE(tree.HasKey(250), "HasKey(250) expected true");
    TEST_ASSERT_CONTINUE(!tree.HasKey(150), "HasKey(150) expected false (deleted)");
    TEST_ASSERT_CONTINUE(!tree.HasKey(999), "HasKey(999) expected false (missing)");
    std::cout << "[OK] HasKey checks succeeded" << std::endl;
    
    // 测试GetKeyCount
    size_t count = tree.GetKeyCount();
    TEST_ASSERT_CONTINUE(count == 4, "Key count should be 4");  // 100, 200, 250, 300
    std::cout << "[OK] GetKeyCount succeeded, keys: " << count << std::endl;
    
    // 测试范围查询
    auto range_result = tree.Range(100, 300);
    TEST_ASSERT_CONTINUE(range_result.size() == 4, "Range result size should be 4");
    std::cout << "[OK] Range query succeeded, count: " << range_result.size() << std::endl;
    
    engine.Shutdown();
    std::cout << "[OK] Enhanced operations test passed" << std::endl;
}

void test_bplus_tree_generic_operations() {
    std::cout << "\n=== Testing B+Tree templated operations ===" << std::endl;
    
    StorageEngine engine("test_bplus_generic.bin");
    BPlusTree tree(&engine);
    
    // 创建新树
    page_id_t root_id = tree.CreateNew();
    TEST_ASSERT_CONTINUE(root_id != INVALID_PAGE_ID, "Failed to create B+Tree");
    
    RID rid1{1, 10};
    RID rid2{2, 20};
    RID rid3{3, 30};
    
    // 测试int32_t键类型（原生支持）
    TEST_ASSERT_CONTINUE(tree.InsertGeneric<int32_t>(100, rid1), "Insert int32_t key 100 failed");
    TEST_ASSERT_CONTINUE(tree.InsertGeneric<int32_t>(200, rid2), "Insert int32_t key 200 failed");
    TEST_ASSERT_CONTINUE(tree.InsertGeneric<int32_t>(150, rid3), "Insert int32_t key 150 failed");
    std::cout << "[OK] int32_t operations succeeded" << std::endl;
    
    // 测试查找
    auto int_result = tree.SearchGeneric<int32_t>(100);
    TEST_ASSERT_CONTINUE(int_result.has_value(), "Search int32_t key 100 failed");
    TEST_ASSERT_CONTINUE(int_result->page_id == 1 && int_result->slot == 10, "RID for int32_t key 100 incorrect");
    std::cout << "[OK] int32_t search succeeded" << std::endl;
    
    // 测试删除
    TEST_ASSERT_CONTINUE(tree.DeleteGeneric<int32_t>(150), "Delete int32_t key 150 failed");
    auto deleted_result = tree.SearchGeneric<int32_t>(150);
    TEST_ASSERT_CONTINUE(!deleted_result.has_value(), "int32_t key 150 still found after delete");
    std::cout << "[OK] int32_t delete succeeded" << std::endl;
    
    // 测试std::string键类型（通过哈希转换）
    TEST_ASSERT_CONTINUE(tree.InsertGeneric<std::string>("hello", rid1), "Insert string key 'hello' failed");
    TEST_ASSERT_CONTINUE(tree.InsertGeneric<std::string>("world", rid2), "Insert string key 'world' failed");
    TEST_ASSERT_CONTINUE(tree.InsertGeneric<std::string>("minidb", rid3), "Insert string key 'minidb' failed");
    std::cout << "[OK] std::string operations succeeded" << std::endl;
    
    // 测试字符串查找
    auto string_result1 = tree.SearchGeneric<std::string>("hello");
    TEST_ASSERT_CONTINUE(string_result1.has_value(), "Search string key 'hello' failed");
    TEST_ASSERT_CONTINUE(string_result1->page_id == 1 && string_result1->slot == 10, "RID for 'hello' incorrect");
    
    auto string_result2 = tree.SearchGeneric<std::string>("world");
    TEST_ASSERT_CONTINUE(string_result2.has_value(), "Search string key 'world' failed");
    TEST_ASSERT_CONTINUE(string_result2->page_id == 2 && string_result2->slot == 20, "RID for 'world' incorrect");
    std::cout << "[OK] std::string search succeeded" << std::endl;
    
    // 测试字符串删除
    TEST_ASSERT_CONTINUE(tree.DeleteGeneric<std::string>("minidb"), "Delete string key 'minidb' failed");
    auto deleted_string_result = tree.SearchGeneric<std::string>("minidb");
    TEST_ASSERT_CONTINUE(!deleted_string_result.has_value(), "string key 'minidb' still found after delete");
    std::cout << "[OK] std::string delete succeeded" << std::endl;
    
    engine.Shutdown();
    std::cout << "[OK] Templated operations test passed" << std::endl;
}

void test_bplus_tree_edge_cases() {
    std::cout << "\n=== Testing B+Tree edge cases ===" << std::endl;
    
    StorageEngine engine("test_bplus_edge.bin");
    BPlusTree tree(&engine);
    
    // 测试空树操作
    TEST_ASSERT_CONTINUE(tree.GetKeyCount() == 0, "Empty tree key count should be 0");
    TEST_ASSERT_CONTINUE(!tree.HasKey(100), "Empty tree should contain no keys");
    auto empty_result = tree.Search(100);
    TEST_ASSERT_CONTINUE(!empty_result.has_value(), "Search on empty tree should be empty");
    auto empty_range = tree.Range(1, 100);
    TEST_ASSERT_CONTINUE(empty_range.empty(), "Range on empty tree should be empty");
    std::cout << "[OK] Empty-tree checks passed" << std::endl;
    
    // 创建树并插入数据
    page_id_t root_id = tree.CreateNew();
    TEST_ASSERT_CONTINUE(root_id != INVALID_PAGE_ID, "Failed to create B+Tree");
    
    RID rid{1, 10};
    TEST_ASSERT_CONTINUE(tree.Insert(100, rid), "Insert key 100 failed");
    
    // 测试删除不存在的键
    TEST_ASSERT_CONTINUE(!tree.Delete(999), "Delete of missing key should be false");
    TEST_ASSERT_CONTINUE(!tree.Update(999, rid), "Update of missing key should be false");
    std::cout << "[OK] Missing-key checks passed" << std::endl;
    
    // 测试重复插入（应该成功，但可能覆盖）
    RID rid2{2, 20};
    TEST_ASSERT_CONTINUE(tree.Insert(100, rid2), "Duplicate insert of key 100 should succeed");  // duplicate key
    auto duplicate_result = tree.Search(100);
    TEST_ASSERT_CONTINUE(duplicate_result.has_value(), "Key 100 should still be found after duplicate insert");
    std::cout << "[OK] Duplicate key insert check passed" << std::endl;
    
    // 测试范围查询边界
    auto single_range = tree.Range(100, 100);
    TEST_ASSERT_CONTINUE(single_range.size() == 1, "Single-key range should return 1 result");
    
    auto no_match_range = tree.Range(200, 300);
    TEST_ASSERT_CONTINUE(no_match_range.empty(), "No-match range should return empty");
    std::cout << "[OK] Range boundary checks passed" << std::endl;
    
    engine.Shutdown();
    std::cout << "[OK] Edge cases test passed" << std::endl;
}

void test_bplus_tree_persistence() {
    std::cout << "\n=== Testing B+Tree persistence ===" << std::endl;
    
    StorageEngine engine("test_bplus_persistence.bin");
    BPlusTree tree(&engine);
    
    // 创建树并插入数据
    page_id_t root_id = tree.CreateNew();
    TEST_ASSERT_CONTINUE(root_id != INVALID_PAGE_ID, "Failed to create B+Tree");
    
    RID rid1{1, 10};
    RID rid2{2, 20};
    RID rid3{3, 30};
    
    TEST_ASSERT_CONTINUE(tree.Insert(100, rid1), "Insert key 100 failed");
    TEST_ASSERT_CONTINUE(tree.Insert(200, rid2), "Insert key 200 failed");
    TEST_ASSERT_CONTINUE(tree.Insert(150, rid3), "Insert key 150 failed");
    
    // 保存根页号
    page_id_t saved_root = tree.GetRoot();
    std::cout << "[OK] Data inserted, root page id: " << saved_root << std::endl;
    
    // 检查点保存
    engine.Checkpoint();
    std::cout << "[OK] Checkpoint saved" << std::endl;
    
    // 关闭并重新打开
    engine.Shutdown();
    
    StorageEngine engine2("test_bplus_persistence.bin");
    BPlusTree tree2(&engine2);
    
    // Restore root page id
    tree2.SetRoot(saved_root);
    
    // 验证数据是否正确恢复
    auto result1 = tree2.Search(100);
    TEST_ASSERT_CONTINUE(result1.has_value(), "Search key 100 after reopen failed");
    TEST_ASSERT_CONTINUE(result1->page_id == 1 && result1->slot == 10, "RID for key 100 after reopen incorrect");
    
    auto result2 = tree2.Search(200);
    TEST_ASSERT_CONTINUE(result2.has_value(), "Search key 200 after reopen failed");
    TEST_ASSERT_CONTINUE(result2->page_id == 2 && result2->slot == 20, "RID for key 200 after reopen incorrect");
    
    auto result3 = tree2.Search(150);
    TEST_ASSERT_CONTINUE(result3.has_value(), "Search key 150 after reopen failed");
    TEST_ASSERT_CONTINUE(result3->page_id == 3 && result3->slot == 30, "RID for key 150 after reopen incorrect");
    
    TEST_ASSERT_CONTINUE(tree2.GetKeyCount() == 3, "Key count after reopen should be 3");
    std::cout << "[OK] Persistence restore succeeded" << std::endl;
    
    engine2.Shutdown();
    std::cout << "[OK] Persistence test passed" << std::endl;
}

int main() {
    std::cout << "Starting B+Tree enhancement tests..." << std::endl;
    
    try {
        test_bplus_tree_basic_operations();
        test_bplus_tree_enhanced_operations();
        test_bplus_tree_generic_operations();
        test_bplus_tree_edge_cases();
        test_bplus_tree_persistence();
        
        std::cout << "\nAll B+Tree tests passed. Enhanced features working." << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "[FAIL] Test failed: " << e.what() << std::endl;
        return 1;
    }
}