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

// 测试root用户角色
bool testRootUserRole() {
    std::cout << "\n=== 测试root用户角色 ===" << std::endl;
    const std::string dbFile = "data/test_root_role_" + std::to_string(time(nullptr)) + ".bin";
    cleanupTestFiles(dbFile);

    StorageEngine storage_engine(dbFile, 16);
    Catalog catalog(&storage_engine);
    AuthService auth_service(&storage_engine, &catalog);

    // 1. 测试root用户登录
    TEST_ASSERT(auth_service.login("root", "root"), "root用户登录应该成功");
    
    // 2. 检查root用户角色
    Role root_role = auth_service.getCurrentUserRole();
    std::cout << "root用户角色: " << static_cast<int>(root_role) << " (应该是0=DBA)" << std::endl;
    TEST_ASSERT(root_role == Role::DBA, "root用户角色应该是DBA");
    
    // 3. 检查root用户权限
    std::cout << "检查root用户权限:" << std::endl;
    std::cout << "  CREATE_USER权限: " << (auth_service.hasPermission(Permission::CREATE_USER) ? "有" : "无") << std::endl;
    std::cout << "  SELECT权限: " << (auth_service.hasPermission(Permission::SELECT) ? "有" : "无") << std::endl;
    std::cout << "  CREATE_TABLE权限: " << (auth_service.hasPermission(Permission::CREATE_TABLE) ? "有" : "无") << std::endl;
    
    // 4. 测试isDBA方法
    TEST_ASSERT(auth_service.isDBA(), "root用户应该是DBA");
    
    // 5. 测试创建用户权限
    TEST_ASSERT(auth_service.hasPermission(Permission::CREATE_USER), "root用户应该有CREATE_USER权限");

    cleanupTestFiles(dbFile);
    return true;
}

int main() {
    std::cout << "=== root用户角色调试测试程序 ===" << std::endl;
    std::cout << "测试root用户角色和权限" << std::endl;
    std::cout << "===============================================" << std::endl;

    bool all_passed = true;

    try {
        all_passed &= testRootUserRole();
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] 测试过程中发生异常: " << e.what() << std::endl;
        all_passed = false;
    }

    std::cout << "\n===============================================" << std::endl;
    if (all_passed) {
        std::cout << "✅ 所有root用户角色测试通过！" << std::endl;
    } else {
        std::cout << "❌ 部分root用户角色测试失败，需要修复问题" << std::endl;
    }

    return all_passed ? 0 : 1;
}
