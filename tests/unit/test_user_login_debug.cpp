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

// 测试用户创建和登录
bool testUserCreationAndLogin() {
    std::cout << "\n=== 测试用户创建和登录 ===" << std::endl;
    const std::string dbFile = "data/test_user_login_" + std::to_string(time(nullptr)) + ".bin";
    cleanupTestFiles(dbFile);

    StorageEngine storage_engine(dbFile, 16);
    Catalog catalog(&storage_engine);
    AuthService auth_service(&storage_engine, &catalog);

    // 1. 测试root用户登录
    TEST_ASSERT(auth_service.login("root", "root"), "root用户登录应该成功");
    
    // 2. 创建devuser
    TEST_ASSERT(auth_service.createUser("devuser", "devpass", Role::DEVELOPER), "创建devuser应该成功");
    
    // 3. 登出root
    auth_service.logout();
    
    // 4. 测试devuser登录
    TEST_ASSERT(auth_service.login("devuser", "devpass"), "devuser登录应该成功");
    
    // 5. 登出devuser
    auth_service.logout();
    
    // 6. 测试错误的密码
    TEST_ASSERT(!auth_service.login("devuser", "wrongpass"), "devuser错误密码应该失败");
    
    // 7. 测试不存在的用户
    TEST_ASSERT(!auth_service.login("nonexistent", "pass"), "不存在的用户应该失败");

    cleanupTestFiles(dbFile);
    return true;
}

int main() {
    std::cout << "=== 用户登录调试测试程序 ===" << std::endl;
    std::cout << "测试用户创建和登录功能" << std::endl;
    std::cout << "===============================================" << std::endl;

    bool all_passed = true;

    try {
        all_passed &= testUserCreationAndLogin();
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] 测试过程中发生异常: " << e.what() << std::endl;
        all_passed = false;
    }

    std::cout << "\n===============================================" << std::endl;
    if (all_passed) {
        std::cout << "✅ 所有用户登录测试通过！" << std::endl;
    } else {
        std::cout << "❌ 部分用户登录测试失败，需要修复问题" << std::endl;
    }

    return all_passed ? 0 : 1;
}
