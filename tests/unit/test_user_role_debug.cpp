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

// 测试用户角色获取
bool testUserRoleRetrieval() {
    std::cout << "\n=== 测试用户角色获取 ===" << std::endl;
    const std::string dbFile = "data/test_user_role_" + std::to_string(time(nullptr)) + ".bin";
    cleanupTestFiles(dbFile);

    StorageEngine storage_engine(dbFile, 16);
    Catalog catalog(&storage_engine);
    AuthService auth_service(&storage_engine, &catalog);

    // 1. 测试root用户角色
    TEST_ASSERT(auth_service.login("root", "root"), "root用户登录应该成功");
    Role root_role = auth_service.getCurrentUserRole();
    std::cout << "root用户角色: " << static_cast<int>(root_role) << " (应该是0=DBA)" << std::endl;
    TEST_ASSERT(root_role == Role::DBA, "root用户角色应该是DBA");
    
    // 2. 创建devuser并测试角色
    TEST_ASSERT(auth_service.createUser("devuser", "devpass", Role::DEVELOPER), "创建devuser应该成功");
    auth_service.logout();
    TEST_ASSERT(auth_service.login("devuser", "devpass"), "devuser登录应该成功");
    Role dev_role = auth_service.getCurrentUserRole();
    std::cout << "devuser角色: " << static_cast<int>(dev_role) << " (应该是1=DEVELOPER)" << std::endl;
    TEST_ASSERT(dev_role == Role::DEVELOPER, "devuser角色应该是DEVELOPER");
    
    // 3. 测试权限检查
    std::cout << "测试devuser权限:" << std::endl;
    std::cout << "  SELECT权限: " << (auth_service.hasPermission(Permission::SELECT) ? "有" : "无") << std::endl;
    std::cout << "  INSERT权限: " << (auth_service.hasPermission(Permission::INSERT) ? "有" : "无") << std::endl;
    std::cout << "  CREATE_USER权限: " << (auth_service.hasPermission(Permission::CREATE_USER) ? "有" : "无") << std::endl;
    
    // 4. 登出并重新登录root测试
    auth_service.logout();
    TEST_ASSERT(auth_service.login("root", "root"), "root用户重新登录应该成功");
    Role root_role2 = auth_service.getCurrentUserRole();
    std::cout << "root用户重新登录后角色: " << static_cast<int>(root_role2) << " (应该是0=DBA)" << std::endl;
    TEST_ASSERT(root_role2 == Role::DBA, "root用户重新登录后角色应该是DBA");

    cleanupTestFiles(dbFile);
    return true;
}

int main() {
    std::cout << "=== 用户角色获取调试测试程序 ===" << std::endl;
    std::cout << "测试用户角色获取和权限检查功能" << std::endl;
    std::cout << "===============================================" << std::endl;

    bool all_passed = true;

    try {
        all_passed &= testUserRoleRetrieval();
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] 测试过程中发生异常: " << e.what() << std::endl;
        all_passed = false;
    }

    std::cout << "\n===============================================" << std::endl;
    if (all_passed) {
        std::cout << "✅ 所有用户角色测试通过！" << std::endl;
    } else {
        std::cout << "❌ 部分用户角色测试失败，需要修复问题" << std::endl;
    }

    return all_passed ? 0 : 1;
}
