#include "../../src/auth/auth_service.h"
#include "../../src/storage/storage_engine.h"
#include "../../src/catalog/catalog.h"
#include <iostream>
#include <string>
#ifdef _WIN32
#include <direct.h>
#define getcwd _getcwd
#else
#include <unistd.h>
#endif

using namespace minidb;

// 测试辅助宏
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            std::cout << "[FAIL] " << message << std::endl; \
            return false; \
        } \
    } while(0)

bool test_storage_based_auth() {
    std::cout << "=== Testing Storage-Based Authentication ===" << std::endl;
    
    try {
        // 打印当前工作目录
        char* cwd = getcwd(nullptr, 0);
        if (cwd) {
            std::cout << "Current working directory: " << cwd << std::endl;
            free(cwd);
        }
        
        // 创建存储引擎和目录，使用正确的路径
        std::string db_file = "data/test_auth_storage.bin";
        std::cout << "Creating storage engine with file: " << db_file << std::endl;
        auto storage_engine = std::make_unique<StorageEngine>(db_file, 16);
        std::cout << "Storage engine created successfully" << std::endl;
        
        auto catalog = std::make_unique<Catalog>(storage_engine.get());
        std::cout << "Catalog created successfully" << std::endl;
        
        // 创建认证服务
        std::cout << "Creating auth service..." << std::endl;
        AuthService auth_service(storage_engine.get(), catalog.get());
        std::cout << "Auth service created successfully" << std::endl;
    
        // 测试root用户登录
        bool login_result = auth_service.login("root", "root");
        std::cout << "Root login result: " << login_result << std::endl;
        TEST_ASSERT(login_result == true, "Root user should be able to login");
        TEST_ASSERT(auth_service.isLoggedIn() == true, "Should be logged in after successful login");
        TEST_ASSERT(auth_service.getCurrentUser() == "root", "Current user should be root");
        TEST_ASSERT(auth_service.isDBA() == true, "Root should be DBA");
        
        // 测试创建新用户
        bool create_result = auth_service.createUser("testuser", "password123", Role::DEVELOPER);
        std::cout << "Create user result: " << create_result << std::endl;
        TEST_ASSERT(create_result == true, "Should be able to create new user as DBA");
        TEST_ASSERT(auth_service.userExists("testuser") == true, "New user should exist");
        
        // 测试新用户登录
        auth_service.logout();
        bool new_user_login = auth_service.login("testuser", "password123");
        std::cout << "New user login result: " << new_user_login << std::endl;
        TEST_ASSERT(new_user_login == true, "New user should be able to login");
        TEST_ASSERT(auth_service.getCurrentUser() == "testuser", "Current user should be testuser");
        TEST_ASSERT(auth_service.getCurrentUserRole() == Role::DEVELOPER, "User should have DEVELOPER role");
        TEST_ASSERT(auth_service.isDBA() == false, "New user should not be DBA");
        
        // 测试权限检查
        TEST_ASSERT(auth_service.hasPermission(Permission::CREATE_TABLE) == true, "Developer should have CREATE_TABLE permission");
        TEST_ASSERT(auth_service.hasPermission(Permission::CREATE_USER) == false, "Developer should not have CREATE_USER permission");
        
        // 测试用户列表
        auth_service.logout();
        auth_service.login("root", "root");
        auto users = auth_service.listUsers();
        std::cout << "User list size: " << users.size() << std::endl;
        for (const auto& user : users) {
            std::cout << "  User: " << user << std::endl;
        }
        TEST_ASSERT(users.size() >= 2, "Should have at least 2 users (root and testuser)");
        
        // 测试创建只读用户
        bool create_readonly = auth_service.createUser("readonly", "readonly123", Role::ANALYST);
        std::cout << "Create readonly user result: " << create_readonly << std::endl;
        TEST_ASSERT(create_readonly == true, "Should be able to create readonly user");
        
        // 测试只读用户权限
        auth_service.logout();
        auth_service.login("readonly", "readonly123");
        TEST_ASSERT(auth_service.hasPermission(Permission::SELECT) == true, "Analyst should have SELECT permission");
        TEST_ASSERT(auth_service.hasPermission(Permission::INSERT) == false, "Analyst should not have INSERT permission");
        TEST_ASSERT(auth_service.hasPermission(Permission::CREATE_TABLE) == false, "Analyst should not have CREATE_TABLE permission");
        
        std::cout << "[OK] Storage-based authentication tests passed!" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cout << "[ERROR] Exception in storage-based auth test: " << e.what() << std::endl;
        return false;
    } catch (...) {
        std::cout << "[ERROR] Unknown exception in storage-based auth test" << std::endl;
        return false;
    }
}

bool test_file_based_auth() {
    std::cout << "=== Testing File-Based Authentication (Fallback) ===" << std::endl;
    
    // 创建认证服务（不使用存储引擎）
    AuthService auth_service;
    
    // 测试admin用户登录（默认用户）
    bool login_result = auth_service.login("admin", "admin123");
    std::cout << "Admin login result: " << login_result << std::endl;
    TEST_ASSERT(login_result == true, "Admin user should be able to login");
    TEST_ASSERT(auth_service.isLoggedIn() == true, "Should be logged in after successful login");
    TEST_ASSERT(auth_service.getCurrentUser() == "admin", "Current user should be admin");
    TEST_ASSERT(auth_service.isDBA() == true, "Admin should be DBA");
    
    std::cout << "[OK] File-based authentication tests passed!" << std::endl;
    return true;
}

int main() {
    std::cout << "Starting Auth Storage Tests..." << std::endl;
    std::cout << "=================================" << std::endl;
    
    bool all_passed = true;
    
    try {
        if (!test_storage_based_auth()) {
            all_passed = false;
        }
        
        if (!test_file_based_auth()) {
            all_passed = false;
        }
        
        std::cout << "=================================" << std::endl;
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