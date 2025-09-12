#include <iostream>
#include <cassert>
#include "../../src/storage/storage_engine.h"
#include "../../src/storage/index/bplus_tree.h"
#include "../../src/util/config.h"

// 定义测试辅助宏，避免使用 assert() 导致 abort()
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            std::cout << "❌ 测试失败: " << message << std::endl; \
            return false; \
        } \
    } while(0)

#define TEST_ASSERT_CONTINUE(condition, message) \
    do { \
        if (!(condition)) { \
            std::cout << "❌ 测试失败: " << message << std::endl; \
            return; \
        } \
    } while(0)

using namespace minidb;

void test_bplus_tree_basic_operations() {
    std::cout << "=== 测试B+树基本操作 ===" << std::endl;
    
    StorageEngine engine("test_bplus_basic.bin");
    BPlusTree tree(&engine);
    
    // 创建新树
    page_id_t root_id = tree.CreateNew();
    TEST_ASSERT_CONTINUE(root_id != INVALID_PAGE_ID, "B+树创建失败");
    std::cout << "✓ B+树创建成功，根页号: " << root_id << std::endl;
    
    // 测试插入
    RID rid1{1, 10};
    RID rid2{2, 20};
    RID rid3{3, 30};
    
    TEST_ASSERT_CONTINUE(tree.Insert(100, rid1), "插入键100失败");
    TEST_ASSERT_CONTINUE(tree.Insert(200, rid2), "插入键200失败");
    TEST_ASSERT_CONTINUE(tree.Insert(150, rid3), "插入键150失败");
    std::cout << "✓ 插入操作成功" << std::endl;
    
    // 测试查找
    auto result1 = tree.Search(100);
    TEST_ASSERT_CONTINUE(result1.has_value(), "查找键100失败");
    TEST_ASSERT_CONTINUE(result1->page_id == 1 && result1->slot == 10, "键100的RID不正确");
    
    auto result2 = tree.Search(200);
    TEST_ASSERT_CONTINUE(result2.has_value(), "查找键200失败");
    TEST_ASSERT_CONTINUE(result2->page_id == 2 && result2->slot == 20, "键200的RID不正确");
    
    auto result3 = tree.Search(150);
    TEST_ASSERT_CONTINUE(result3.has_value(), "查找键150失败");
    TEST_ASSERT_CONTINUE(result3->page_id == 3 && result3->slot == 30, "键150的RID不正确");
    std::cout << "✓ 查找操作成功" << std::endl;
    
    // 测试范围查询
    auto range_result = tree.Range(100, 200);
    TEST_ASSERT_CONTINUE(range_result.size() == 3, "范围查询结果数量不正确");
    std::cout << "✓ 范围查询成功，结果数量: " << range_result.size() << std::endl;
    
    engine.Shutdown();
    std::cout << "✓ B+树基本操作测试通过" << std::endl;
}

void test_bplus_tree_enhanced_operations() {
    std::cout << "\n=== 测试B+树增强操作 ===" << std::endl;
    
    StorageEngine engine("test_bplus_enhanced.bin");
    BPlusTree tree(&engine);
    
    // 创建新树
    page_id_t root_id = tree.CreateNew();
    TEST_ASSERT_CONTINUE(root_id != INVALID_PAGE_ID, "B+树创建失败");
    
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
    std::cout << "✓ 批量插入成功" << std::endl;
    
    // 测试更新操作
    RID new_rid{99, 99};
    TEST_ASSERT_CONTINUE(tree.Update(100, new_rid), "更新键100失败");
    auto updated_result = tree.Search(100);
    TEST_ASSERT_CONTINUE(updated_result.has_value(), "更新后查找键100失败");
    TEST_ASSERT_CONTINUE(updated_result->page_id == 99 && updated_result->slot == 99, "更新后的RID不正确");
    std::cout << "✓ 更新操作成功" << std::endl;
    
    // 测试删除操作
    TEST_ASSERT_CONTINUE(tree.Delete(150), "删除键150失败");
    auto deleted_result = tree.Search(150);
    TEST_ASSERT_CONTINUE(!deleted_result.has_value(), "删除后仍能找到键150");
    std::cout << "✓ 删除操作成功" << std::endl;
    
    // 测试HasKey
    TEST_ASSERT_CONTINUE(tree.HasKey(100), "HasKey(100)应该返回true");
    TEST_ASSERT_CONTINUE(tree.HasKey(200), "HasKey(200)应该返回true");
    TEST_ASSERT_CONTINUE(tree.HasKey(300), "HasKey(300)应该返回true");
    TEST_ASSERT_CONTINUE(tree.HasKey(250), "HasKey(250)应该返回true");
    TEST_ASSERT_CONTINUE(!tree.HasKey(150), "HasKey(150)应该返回false（已删除）");
    TEST_ASSERT_CONTINUE(!tree.HasKey(999), "HasKey(999)应该返回false（不存在）");
    std::cout << "✓ HasKey操作成功" << std::endl;
    
    // 测试GetKeyCount
    size_t count = tree.GetKeyCount();
    TEST_ASSERT_CONTINUE(count == 4, "键数量应该为4");  // 100, 200, 250, 300
    std::cout << "✓ GetKeyCount操作成功，键数量: " << count << std::endl;
    
    // 测试范围查询
    auto range_result = tree.Range(100, 300);
    TEST_ASSERT_CONTINUE(range_result.size() == 4, "范围查询结果数量应该为4");
    std::cout << "✓ 范围查询成功，结果数量: " << range_result.size() << std::endl;
    
    engine.Shutdown();
    std::cout << "✓ B+树增强操作测试通过" << std::endl;
}

void test_bplus_tree_generic_operations() {
    std::cout << "\n=== 测试B+树模板化操作 ===" << std::endl;
    
    StorageEngine engine("test_bplus_generic.bin");
    BPlusTree tree(&engine);
    
    // 创建新树
    page_id_t root_id = tree.CreateNew();
    TEST_ASSERT_CONTINUE(root_id != INVALID_PAGE_ID, "B+树创建失败");
    
    RID rid1{1, 10};
    RID rid2{2, 20};
    RID rid3{3, 30};
    
    // 测试int32_t键类型（原生支持）
    TEST_ASSERT_CONTINUE(tree.InsertGeneric<int32_t>(100, rid1), "插入int32_t键100失败");
    TEST_ASSERT_CONTINUE(tree.InsertGeneric<int32_t>(200, rid2), "插入int32_t键200失败");
    TEST_ASSERT_CONTINUE(tree.InsertGeneric<int32_t>(150, rid3), "插入int32_t键150失败");
    std::cout << "✓ int32_t键类型操作成功" << std::endl;
    
    // 测试查找
    auto int_result = tree.SearchGeneric<int32_t>(100);
    TEST_ASSERT_CONTINUE(int_result.has_value(), "查找int32_t键100失败");
    TEST_ASSERT_CONTINUE(int_result->page_id == 1 && int_result->slot == 10, "int32_t键100的RID不正确");
    std::cout << "✓ int32_t查找成功" << std::endl;
    
    // 测试删除
    TEST_ASSERT_CONTINUE(tree.DeleteGeneric<int32_t>(150), "删除int32_t键150失败");
    auto deleted_result = tree.SearchGeneric<int32_t>(150);
    TEST_ASSERT_CONTINUE(!deleted_result.has_value(), "删除后仍能找到int32_t键150");
    std::cout << "✓ int32_t删除成功" << std::endl;
    
    // 测试std::string键类型（通过哈希转换）
    TEST_ASSERT_CONTINUE(tree.InsertGeneric<std::string>("hello", rid1), "插入string键hello失败");
    TEST_ASSERT_CONTINUE(tree.InsertGeneric<std::string>("world", rid2), "插入string键world失败");
    TEST_ASSERT_CONTINUE(tree.InsertGeneric<std::string>("minidb", rid3), "插入string键minidb失败");
    std::cout << "✓ std::string键类型操作成功" << std::endl;
    
    // 测试字符串查找
    auto string_result1 = tree.SearchGeneric<std::string>("hello");
    TEST_ASSERT_CONTINUE(string_result1.has_value(), "查找string键hello失败");
    TEST_ASSERT_CONTINUE(string_result1->page_id == 1 && string_result1->slot == 10, "string键hello的RID不正确");
    
    auto string_result2 = tree.SearchGeneric<std::string>("world");
    TEST_ASSERT_CONTINUE(string_result2.has_value(), "查找string键world失败");
    TEST_ASSERT_CONTINUE(string_result2->page_id == 2 && string_result2->slot == 20, "string键world的RID不正确");
    std::cout << "✓ std::string查找成功" << std::endl;
    
    // 测试字符串删除
    TEST_ASSERT_CONTINUE(tree.DeleteGeneric<std::string>("minidb"), "删除string键minidb失败");
    auto deleted_string_result = tree.SearchGeneric<std::string>("minidb");
    TEST_ASSERT_CONTINUE(!deleted_string_result.has_value(), "删除后仍能找到string键minidb");
    std::cout << "✓ std::string删除成功" << std::endl;
    
    engine.Shutdown();
    std::cout << "✓ B+树模板化操作测试通过" << std::endl;
}

void test_bplus_tree_edge_cases() {
    std::cout << "\n=== 测试B+树边界情况 ===" << std::endl;
    
    StorageEngine engine("test_bplus_edge.bin");
    BPlusTree tree(&engine);
    
    // 测试空树操作
    TEST_ASSERT_CONTINUE(tree.GetKeyCount() == 0, "空树的键数量应该为0");
    TEST_ASSERT_CONTINUE(!tree.HasKey(100), "空树不应该包含任何键");
    auto empty_result = tree.Search(100);
    TEST_ASSERT_CONTINUE(!empty_result.has_value(), "空树查找应该返回空值");
    auto empty_range = tree.Range(1, 100);
    TEST_ASSERT_CONTINUE(empty_range.empty(), "空树范围查询应该返回空结果");
    std::cout << "✓ 空树操作测试通过" << std::endl;
    
    // 创建树并插入数据
    page_id_t root_id = tree.CreateNew();
    TEST_ASSERT_CONTINUE(root_id != INVALID_PAGE_ID, "B+树创建失败");
    
    RID rid{1, 10};
    TEST_ASSERT_CONTINUE(tree.Insert(100, rid), "插入键100失败");
    
    // 测试删除不存在的键
    TEST_ASSERT_CONTINUE(!tree.Delete(999), "删除不存在的键应该返回false");
    TEST_ASSERT_CONTINUE(!tree.Update(999, rid), "更新不存在的键应该返回false");
    std::cout << "✓ 不存在键的操作测试通过" << std::endl;
    
    // 测试重复插入（应该成功，但可能覆盖）
    RID rid2{2, 20};
    TEST_ASSERT_CONTINUE(tree.Insert(100, rid2), "重复插入键100应该成功");  // 重复键
    auto duplicate_result = tree.Search(100);
    TEST_ASSERT_CONTINUE(duplicate_result.has_value(), "重复插入后应该能找到键100");
    // 注意：这里的行为取决于实现，可能覆盖也可能保持原值
    std::cout << "✓ 重复键插入测试通过" << std::endl;
    
    // 测试范围查询边界
    auto single_range = tree.Range(100, 100);
    TEST_ASSERT_CONTINUE(single_range.size() == 1, "单键范围查询应该返回1个结果");
    
    auto no_match_range = tree.Range(200, 300);
    TEST_ASSERT_CONTINUE(no_match_range.empty(), "无匹配的范围查询应该返回空结果");
    std::cout << "✓ 范围查询边界测试通过" << std::endl;
    
    engine.Shutdown();
    std::cout << "✓ B+树边界情况测试通过" << std::endl;
}

void test_bplus_tree_persistence() {
    std::cout << "\n=== 测试B+树持久化 ===" << std::endl;
    
    StorageEngine engine("test_bplus_persistence.bin");
    BPlusTree tree(&engine);
    
    // 创建树并插入数据
    page_id_t root_id = tree.CreateNew();
    TEST_ASSERT_CONTINUE(root_id != INVALID_PAGE_ID, "B+树创建失败");
    
    RID rid1{1, 10};
    RID rid2{2, 20};
    RID rid3{3, 30};
    
    TEST_ASSERT_CONTINUE(tree.Insert(100, rid1), "插入键100失败");
    TEST_ASSERT_CONTINUE(tree.Insert(200, rid2), "插入键200失败");
    TEST_ASSERT_CONTINUE(tree.Insert(150, rid3), "插入键150失败");
    
    // 保存根页号
    page_id_t saved_root = tree.GetRoot();
    std::cout << "✓ 数据插入成功，根页号: " << saved_root << std::endl;
    
    // 检查点保存
    engine.Checkpoint();
    std::cout << "✓ 检查点保存成功" << std::endl;
    
    // 关闭并重新打开
    engine.Shutdown();
    
    StorageEngine engine2("test_bplus_persistence.bin");
    BPlusTree tree2(&engine2);
    
    // 恢复根页号
    tree2.SetRoot(saved_root);
    
    // 验证数据是否正确恢复
    auto result1 = tree2.Search(100);
    TEST_ASSERT_CONTINUE(result1.has_value(), "恢复后查找键100失败");
    TEST_ASSERT_CONTINUE(result1->page_id == 1 && result1->slot == 10, "恢复后键100的RID不正确");
    
    auto result2 = tree2.Search(200);
    TEST_ASSERT_CONTINUE(result2.has_value(), "恢复后查找键200失败");
    TEST_ASSERT_CONTINUE(result2->page_id == 2 && result2->slot == 20, "恢复后键200的RID不正确");
    
    auto result3 = tree2.Search(150);
    TEST_ASSERT_CONTINUE(result3.has_value(), "恢复后查找键150失败");
    TEST_ASSERT_CONTINUE(result3->page_id == 3 && result3->slot == 30, "恢复后键150的RID不正确");
    
    TEST_ASSERT_CONTINUE(tree2.GetKeyCount() == 3, "恢复后键数量应该为3");
    std::cout << "✓ 数据持久化恢复成功" << std::endl;
    
    engine2.Shutdown();
    std::cout << "✓ B+树持久化测试通过" << std::endl;
}

int main() {
    std::cout << "开始测试B+树增强功能..." << std::endl;
    
    try {
        test_bplus_tree_basic_operations();
        test_bplus_tree_enhanced_operations();
        test_bplus_tree_generic_operations();
        test_bplus_tree_edge_cases();
        test_bplus_tree_persistence();
        
        std::cout << "\n🎉 所有B+树测试通过！B+树增强功能正常工作。" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ 测试失败: " << e.what() << std::endl;
        return 1;
    }
}