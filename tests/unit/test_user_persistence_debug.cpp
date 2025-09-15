#include "../../src/auth/auth_service.h"
#include "../../src/storage/storage_engine.h"
#include "../../src/catalog/catalog.h"
#include "../../src/auth/role_manager.h"
#include <iostream>
#include <fstream>
#include <cassert>

using namespace minidb;

// 测试辅助宏
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            std::cout << "[FAIL] " << message << std::endl; \
            return false; \
        } else { \
            std::cout << "[PASS] " << message << std::endl; \
        } \
    } while(0)

// 测试用户数据持久化
bool testUserPersistence() {
    std::cout << "\n=== Testing User Data Persistence ===" << std::endl;
    
    const std::string dbFile = "data/test_user_persistence.bin";
    
    // 第一次：创建用户数据
    std::cout << "\n--- First Run: Creating Users ---" << std::endl;
    {
        StorageEngine storage_engine(dbFile, 16);
        Catalog catalog(&storage_engine);
        AuthService auth_service(&storage_engine, &catalog);
        
        // 先登录root用户
        TEST_ASSERT(auth_service.login("root", "root"), "Login as root");
        
        // 创建用户
        TEST_ASSERT(auth_service.createUser("testuser1", "password1", Role::DEVELOPER), 
                    "Create testuser1");
        TEST_ASSERT(auth_service.createUser("testuser2", "password2", Role::ANALYST), 
                    "Create testuser2");
        
        // 验证用户存在
        TEST_ASSERT(auth_service.userExists("testuser1"), "testuser1 exists");
        TEST_ASSERT(auth_service.userExists("testuser2"), "testuser2 exists");
        
        // 获取所有用户
        auto users = auth_service.getAllUsers();
        std::cout << "[INFO] Users created: " << users.size() << std::endl;
        for (const auto& user : users) {
            std::cout << "  - " << user.username << " (Role: " << 
                RoleManager().roleToString(user.role) << ")" << std::endl;
        }
    }
    
    // 第二次：重新加载并验证
    std::cout << "\n--- Second Run: Loading Users ---" << std::endl;
    {
        StorageEngine storage_engine(dbFile, 16);
        Catalog catalog(&storage_engine);
        AuthService auth_service(&storage_engine, &catalog);
        
        // 验证用户仍然存在
        TEST_ASSERT(auth_service.userExists("testuser1"), "testuser1 persisted");
        TEST_ASSERT(auth_service.userExists("testuser2"), "testuser2 persisted");
        
        // 验证认证
        TEST_ASSERT(auth_service.login("testuser1", "password1"), "testuser1 authentication");
        TEST_ASSERT(auth_service.login("testuser2", "password2"), "testuser2 authentication");
        
        // 获取所有用户
        auto users = auth_service.getAllUsers();
        std::cout << "[INFO] Users loaded: " << users.size() << std::endl;
        for (const auto& user : users) {
            std::cout << "  - " << user.username << " (Role: " << 
                RoleManager().roleToString(user.role) << ")" << std::endl;
        }
    }
    
    return true;
}

int main() {
    std::cout << "Starting User Persistence Debug Test..." << std::endl;
    std::cout << "=====================================" << std::endl;
    
    bool all_passed = true;
    
    try {
        if (!testUserPersistence()) {
            all_passed = false;
        }
        
        std::cout << "\n=====================================" << std::endl;
        if (all_passed) {
            std::cout << "=== All tests passed! ===" << std::endl;
            return 0;
        } else {
            std::cout << "=== Some tests failed! ===" << std::endl;
            return 1;
        }
    } catch (const std::exception& e) {
        std::cout << "[ERROR] Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
