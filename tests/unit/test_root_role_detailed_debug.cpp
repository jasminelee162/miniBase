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

// 测试root用户角色详细调试
bool testRootUserRoleDetailed() {
    std::cout << "\n=== 测试root用户角色详细调试 ===" << std::endl;
    const std::string dbFile = "data/test_root_role_detailed_" + std::to_string(time(nullptr)) + ".bin";
    cleanupTestFiles(dbFile);

    StorageEngine storage_engine(dbFile, 16);
    Catalog catalog(&storage_engine);
    AuthService auth_service(&storage_engine, &catalog);

    // 1. 测试root用户登录
    TEST_ASSERT(auth_service.login("root", "root"), "root用户登录应该成功");
    
    // 2. 检查登录状态
    std::cout << "登录状态: " << (auth_service.isLoggedIn() ? "已登录" : "未登录") << std::endl;
    std::cout << "当前用户: " << auth_service.getCurrentUser() << std::endl;
    
    // 3. 检查root用户角色
    Role root_role = auth_service.getCurrentUserRole();
    std::cout << "root用户角色: " << static_cast<int>(root_role) << " (应该是0=DBA)" << std::endl;
    TEST_ASSERT(root_role == Role::DBA, "root用户角色应该是DBA");
    
    // 4. 检查isDBA方法
    bool is_dba = auth_service.isDBA();
    std::cout << "isDBA()结果: " << (is_dba ? "是" : "否") << std::endl;
    TEST_ASSERT(is_dba, "root用户应该是DBA");
    
    // 5. 检查权限
    std::cout << "检查root用户权限:" << std::endl;
    std::cout << "  CREATE_USER权限: " << (auth_service.hasPermission(Permission::CREATE_USER) ? "有" : "无") << std::endl;
    std::cout << "  SELECT权限: " << (auth_service.hasPermission(Permission::SELECT) ? "有" : "无") << std::endl;
    std::cout << "  CREATE_TABLE权限: " << (auth_service.hasPermission(Permission::CREATE_TABLE) ? "有" : "无") << std::endl;
    
    // 6. 测试创建用户
    std::cout << "\n测试创建用户..." << std::endl;
    bool create_result = auth_service.createUser("testuser", "testpass", Role::DEVELOPER);
    std::cout << "创建用户结果: " << (create_result ? "成功" : "失败") << std::endl;
    TEST_ASSERT(create_result, "root用户应该能够创建其他用户");

    cleanupTestFiles(dbFile);
    return true;
}

int main() {
    std::cout << "=== root用户角色详细调试测试程序 ===" << std::endl;
    std::cout << "测试root用户角色和权限的详细过程" << std::endl;
    std::cout << "===============================================" << std::endl;

    bool all_passed = true;

    try {
        all_passed &= testRootUserRoleDetailed();
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] 测试过程中发生异常: " << e.what() << std::endl;
        all_passed = false;
    }

    std::cout << "\n===============================================" << std::endl;
    if (all_passed) {
        std::cout << "✅ 所有root用户角色详细调试测试通过！" << std::endl;
    } else {
        std::cout << "❌ 部分root用户角色详细调试测试失败，需要修复问题" << std::endl;
    }

    return all_passed ? 0 : 1;
}
