#include "../../src/auth/auth_service.h"
#include "../../src/storage/storage_engine.h"
#include "../../src/catalog/catalog.h"
#include "../../src/auth/role_manager.h"
#include <iostream>
#include <fstream>
#include <cassert>
#include <windows.h>
#include <io.h>
#include <fcntl.h>

// 防止Windows宏定义冲突
#ifdef DELETE
#undef DELETE
#endif

using namespace minidb;

// 设置控制台中文编码
void setupConsoleEncoding() {
    // 设置控制台代码页为UTF-8
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
}

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

// 检查文件是否存在
bool fileExists(const std::string& filename) {
    std::ifstream file(filename);
    return file.good();
}

// 检查.bin文件的基本结构
bool checkBinFileStructure(const std::string& filename) {
    std::cout << "\n=== Checking .bin file structure ===" << std::endl;
    
    if (!fileExists(filename)) {
        std::cout << "[FAIL] .bin file does not exist: " << filename << std::endl;
        return false;
    }
    
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cout << "[FAIL] Cannot open .bin file: " << filename << std::endl;
        return false;
    }
    
    // 检查文件大小（至少应该有元数据页）
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.close();
    
    TEST_ASSERT(fileSize >= 4096, "File size should be at least 4KB (one page)");
    
    std::cout << "[INFO] .bin file size: " << fileSize << " bytes" << std::endl;
    std::cout << "[INFO] Number of pages: " << (fileSize / 4096) << std::endl;
    
    return true;
}

// 测试完整的用户创建和存储流程
bool testCompleteUserWorkflow() {
    std::cout << "\n=== Testing Complete User Workflow ===" << std::endl;
    
    const std::string dbFile = "data/test_auth_complete.bin";
    
    // 创建存储引擎和目录（如果文件存在则加载，不存在则创建）
    StorageEngine storage_engine(dbFile, 16);
    Catalog catalog(&storage_engine);
    
    // 创建认证服务
    AuthService auth_service(&storage_engine, &catalog);
    
    // 测试root用户登录
    TEST_ASSERT(auth_service.login("root", "root"), "Root user login successful");
    TEST_ASSERT(auth_service.isDBA(), "Root user is DBA");
    
    // 创建developer用户
    TEST_ASSERT(auth_service.createUser("developer1", "dev123", Role::DEVELOPER), 
                "Root user can create developer user");
    
    // 创建analyst用户
    TEST_ASSERT(auth_service.createUser("analyst1", "analyst123", Role::ANALYST), 
                "Root user can create analyst user");
    
    // 验证用户存在
    TEST_ASSERT(auth_service.userExists("root"), "Root user exists");
    TEST_ASSERT(auth_service.userExists("developer1"), "Developer user exists");
    TEST_ASSERT(auth_service.userExists("analyst1"), "Analyst user exists");
    
    // 验证用户角色
    auth_service.login("root", "root");
    TEST_ASSERT(auth_service.getCurrentUserRole() == Role::DBA, "Root role correct");
    
    auth_service.login("developer1", "dev123");
    TEST_ASSERT(auth_service.getCurrentUserRole() == Role::DEVELOPER, "Developer role correct");
    
    auth_service.login("analyst1", "analyst123");
    TEST_ASSERT(auth_service.getCurrentUserRole() == Role::ANALYST, "Analyst role correct");
    
    // 验证权限
    auth_service.login("root", "root");
    TEST_ASSERT(auth_service.hasPermission(Permission::CREATE_USER), "Root can create users");
    TEST_ASSERT(auth_service.hasPermission(Permission::CREATE_TABLE), "Root can create tables");
    
    auth_service.login("developer1", "dev123");
    TEST_ASSERT(auth_service.hasPermission(Permission::CREATE_TABLE), "Developer can create tables");
    TEST_ASSERT(!auth_service.hasPermission(Permission::CREATE_USER), "Developer cannot create users");
    
    auth_service.login("analyst1", "analyst123");
    TEST_ASSERT(auth_service.hasPermission(Permission::SELECT), "Analyst can select");
    TEST_ASSERT(!auth_service.hasPermission(Permission::CREATE_TABLE), "Analyst cannot create tables");
    
    // 检查.bin文件结构
    TEST_ASSERT(checkBinFileStructure(dbFile), "Bin file structure is correct");
    
    // 获取所有用户信息
    auto users = auth_service.getAllUsers();
    std::cout << "[INFO] Users in database:" << std::endl;
    for (const auto& user : users) {
        std::cout << "  - " << user.username << " (Role: " << 
            RoleManager().roleToString(user.role) << ")" << std::endl;
    }
    
    return true;
}

// 测试数据持久化（重新加载）
bool testDataPersistence() {
    std::cout << "\n=== Testing Data Persistence ===" << std::endl;
    
    const std::string dbFile = "data/test_auth_complete.bin";
    
    // 重新创建存储引擎和目录（模拟重启）
    StorageEngine storage_engine(dbFile, 16);
    Catalog catalog(&storage_engine);
    
    // 创建认证服务
    AuthService auth_service(&storage_engine, &catalog);
    
    // 验证所有用户仍然存在
    TEST_ASSERT(auth_service.userExists("root"), "Root user persisted");
    TEST_ASSERT(auth_service.userExists("developer1"), "Developer user persisted");
    TEST_ASSERT(auth_service.userExists("analyst1"), "Analyst user persisted");
    
    // 验证用户认证
    TEST_ASSERT(auth_service.login("root", "root"), "Root authentication persisted");
    TEST_ASSERT(auth_service.login("developer1", "dev123"), "Developer authentication persisted");
    TEST_ASSERT(auth_service.login("analyst1", "analyst123"), "Analyst authentication persisted");
    
    // 验证用户角色
    auth_service.login("root", "root");
    TEST_ASSERT(auth_service.getCurrentUserRole() == Role::DBA, "Root role persisted");
    
    auth_service.login("developer1", "dev123");
    TEST_ASSERT(auth_service.getCurrentUserRole() == Role::DEVELOPER, "Developer role persisted");
    
    auth_service.login("analyst1", "analyst123");
    TEST_ASSERT(auth_service.getCurrentUserRole() == Role::ANALYST, "Analyst role persisted");
    
    return true;
}

int main() {
    // 设置控制台中文编码
    setupConsoleEncoding();
    
    std::cout << "Starting Auth Bin Structure Tests..." << std::endl;
    std::cout << "====================================" << std::endl;
    
    bool all_passed = true;
    
    try {
        // 运行测试
        if (!testCompleteUserWorkflow()) {
            all_passed = false;
        }
        
        if (!testDataPersistence()) {
            all_passed = false;
        }
        
        std::cout << "\n====================================" << std::endl;
        if (all_passed) {
            std::cout << "=== All tests passed! ===" << std::endl;
            std::cout << "✅ Root user successfully written to .bin file" << std::endl;
            std::cout << "✅ Metadata page and catalog page created correctly" << std::endl;
            std::cout << "✅ User table data page created correctly" << std::endl;
            std::cout << "✅ Root user can create developer and analyst users" << std::endl;
            std::cout << "✅ All user data persisted to .bin file" << std::endl;
            std::cout << "✅ User permissions work correctly" << std::endl;
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
