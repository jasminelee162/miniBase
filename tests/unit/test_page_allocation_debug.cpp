#include "../../src/auth/auth_service.h"
#include "../../src/storage/storage_engine.h"
#include "../../src/catalog/catalog.h"
#include <iostream>
#include <string>
#include <vector>
#include <ctime>
#include <cstdio> // For std::remove

using namespace minidb;

#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            std::cout << "[FAIL] " << message << std::endl; \
            return false; \
        } else { \
            std::cout << "[PASS] " << message << std::endl; \
        } \
    } while(0)

// Helper to clean up test files
void cleanupTestFiles(const std::string& dbFile) {
    std::remove(dbFile.c_str());
}

// 测试页面分配调试
bool testPageAllocationDebug() {
    std::cout << "\n=== 测试页面分配调试 ===" << std::endl;
    const std::string dbFile = "data/test_page_allocation_debug_" + std::to_string(time(nullptr)) + ".bin";
    cleanupTestFiles(dbFile);

    StorageEngine storage_engine(dbFile, 16);
    Catalog catalog(&storage_engine);
    AuthService auth_service(&storage_engine, &catalog);

    // 1. 登录root用户
    TEST_ASSERT(auth_service.login("root", "root"), "root用户登录应该成功");
    
    // 2. 检查初始状态
    std::cout << "初始状态 - GetNextPageId: " << storage_engine.GetNextPageId() << std::endl;
    
    // 3. 创建第一张表
    std::cout << "\n创建第一张表..." << std::endl;
    std::vector<Column> columns = {{"id", "INT", -1}, {"name", "VARCHAR", 50}};
    catalog.CreateTable("table1", columns, "root");
    std::cout << "第一张表创建完成" << std::endl;
    std::cout << "创建表后 - GetNextPageId: " << storage_engine.GetNextPageId() << std::endl;
    
    // 4. 创建第二张表
    std::cout << "\n创建第二张表..." << std::endl;
    catalog.CreateTable("table2", columns, "root");
    std::cout << "第二张表创建完成" << std::endl;
    std::cout << "创建表后 - GetNextPageId: " << storage_engine.GetNextPageId() << std::endl;
    
    // 5. 创建第三张表
    std::cout << "\n创建第三张表..." << std::endl;
    catalog.CreateTable("table3", columns, "root");
    std::cout << "第三张表创建完成" << std::endl;
    std::cout << "创建表后 - GetNextPageId: " << storage_engine.GetNextPageId() << std::endl;
    
    // 6. 检查用户状态
    std::cout << "\n检查用户状态..." << std::endl;
    std::cout << "当前用户: " << auth_service.getCurrentUser() << std::endl;
    std::cout << "用户角色: " << static_cast<int>(auth_service.getCurrentUserRole()) << std::endl;
    std::cout << "是否DBA: " << (auth_service.isDBA() ? "是" : "否") << std::endl;
    
    // 7. 尝试创建用户
    std::cout << "\n尝试创建用户..." << std::endl;
    bool create_result = auth_service.createUser("testuser", "testpass", Role::DEVELOPER);
    std::cout << "创建用户结果: " << (create_result ? "成功" : "失败") << std::endl;
    TEST_ASSERT(create_result, "应该能够创建用户");

    cleanupTestFiles(dbFile);
    return true;
}

int main() {
    std::cout << "=== 页面分配调试测试程序 ===" << std::endl;
    std::cout << "调试页面分配和next_page_id更新问题" << std::endl;
    std::cout << "===============================================" << std::endl;

    bool all_passed = true;

    try {
        all_passed &= testPageAllocationDebug();
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] 测试过程中发生异常: " << e.what() << std::endl;
        all_passed = false;
    }

    std::cout << "\n===============================================" << std::endl;
    if (all_passed) {
        std::cout << "✅ 所有测试通过！" << std::endl;
    } else {
        std::cout << "❌ 部分测试失败，需要修复问题" << std::endl;
    }

    return all_passed ? 0 : 1;
}
