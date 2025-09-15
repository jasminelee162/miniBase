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

// 测试表可见性控制调试
bool testTableVisibilityDebug() {
    std::cout << "\n=== 测试表可见性控制调试 ===" << std::endl;
    const std::string dbFile = "data/test_table_visibility_debug_" + std::to_string(time(nullptr)) + ".bin";
    cleanupTestFiles(dbFile);

    StorageEngine storage_engine(dbFile, 16);
    Catalog catalog(&storage_engine);
    AuthService auth_service(&storage_engine, &catalog);

    // 1. 以root用户登录
    std::cout << "步骤1: 登录root用户" << std::endl;
    bool login_result = auth_service.login("root", "root");
    std::cout << "登录结果: " << (login_result ? "成功" : "失败") << std::endl;
    TEST_ASSERT(login_result, "root用户登录应该成功");
    
    // 2. 检查登录状态
    std::cout << "步骤2: 检查登录状态" << std::endl;
    std::cout << "当前用户: " << auth_service.getCurrentUser() << std::endl;
    std::cout << "是否已登录: " << (auth_service.isLoggedIn() ? "是" : "否") << std::endl;
    
    // 3. 检查用户角色
    std::cout << "步骤3: 检查用户角色" << std::endl;
    Role current_role = auth_service.getCurrentUserRole();
    std::cout << "当前用户角色: " << static_cast<int>(current_role) << " (DBA=" << static_cast<int>(Role::DBA) << ")" << std::endl;
    std::cout << "是否DBA: " << (auth_service.isDBA() ? "是" : "否") << std::endl;
    
    // 4. 检查权限
    std::cout << "步骤4: 检查CREATE_USER权限" << std::endl;
    bool has_create_user = auth_service.hasPermission(Permission::CREATE_USER);
    std::cout << "是否有CREATE_USER权限: " << (has_create_user ? "是" : "否") << std::endl;
    
    // 5. 创建一些业务表
    std::cout << "步骤5: 创建业务表" << std::endl;
    std::vector<Column> columns = {{"id", "INT", -1}, {"name", "VARCHAR", 50}};
    catalog.CreateTable("users", columns, "root");
    catalog.CreateTable("orders", columns, "root");
    catalog.CreateTable("products", columns, "root");
    std::cout << "业务表创建完成" << std::endl;
    
    // 6. 再次检查登录状态和角色
    std::cout << "步骤6: 创建表后检查状态" << std::endl;
    std::cout << "当前用户: " << auth_service.getCurrentUser() << std::endl;
    std::cout << "是否已登录: " << (auth_service.isLoggedIn() ? "是" : "否") << std::endl;
    current_role = auth_service.getCurrentUserRole();
    std::cout << "当前用户角色: " << static_cast<int>(current_role) << std::endl;
    std::cout << "是否DBA: " << (auth_service.isDBA() ? "是" : "否") << std::endl;
    has_create_user = auth_service.hasPermission(Permission::CREATE_USER);
    std::cout << "是否有CREATE_USER权限: " << (has_create_user ? "是" : "否") << std::endl;
    
    // 7. 尝试创建DEVELOPER用户
    std::cout << "步骤7: 创建DEVELOPER用户" << std::endl;
    bool create_result = auth_service.createUser("devuser", "devpass", Role::DEVELOPER);
    std::cout << "创建用户结果: " << (create_result ? "成功" : "失败") << std::endl;
    TEST_ASSERT(create_result, "创建devuser应该成功");

    cleanupTestFiles(dbFile);
    return true;
}

int main() {
    std::cout << "=== 表可见性控制调试测试程序 ===" << std::endl;
    std::cout << "调试表可见性控制功能中的问题" << std::endl;
    std::cout << "===============================================" << std::endl;

    bool all_passed = true;

    try {
        all_passed &= testTableVisibilityDebug();
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] 测试过程中发生异常: " << e.what() << std::endl;
        all_passed = false;
    }

    std::cout << "\n===============================================" << std::endl;
    if (all_passed) {
        std::cout << "✅ 所有调试测试通过！" << std::endl;
    } else {
        std::cout << "❌ 部分调试测试失败，需要修复问题" << std::endl;
    }

    return all_passed ? 0 : 1;
}
