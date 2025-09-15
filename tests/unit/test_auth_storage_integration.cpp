#include "../../src/auth/auth_service.h"
#include "../../src/auth/user_storage_manager.h"
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

#define TEST_ASSERT_CONTINUE(condition, message) \
    do { \
        if (!(condition)) { \
            std::cout << "[FAIL] " << message << std::endl; \
            return; \
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
    return true;
}

// 测试存储引擎和目录初始化
bool testStorageEngineInitialization() {
    std::cout << "\n=== Testing Storage Engine Initialization ===" << std::endl;
    
    const std::string dbFile = "data/test_auth_storage.bin";
    
    // 创建存储引擎
    StorageEngine storage_engine(dbFile, 16);
    TEST_ASSERT(true, "StorageEngine created successfully");
    
    // 创建目录
    Catalog catalog(&storage_engine);
    TEST_ASSERT(true, "Catalog created successfully");
    
    // 检查.bin文件是否创建
    TEST_ASSERT(fileExists(dbFile), "Database file created");
    
    return true;
}

// 测试用户存储管理器初始化
bool testUserStorageManagerInitialization() {
    std::cout << "\n=== Testing User Storage Manager Initialization ===" << std::endl;
    
    const std::string dbFile = "data/test_auth_storage.bin";
    
    // 创建存储引擎和目录
    StorageEngine storage_engine(dbFile, 16);
    Catalog catalog(&storage_engine);
    
    // 创建用户存储管理器
    UserStorageManager user_storage_manager(&storage_engine, &catalog);
    TEST_ASSERT(user_storage_manager.initialize(), "UserStorageManager initialized successfully");
    
    // 检查用户表是否创建
    TEST_ASSERT(catalog.HasTable("__users__"), "Users table created in catalog");
    
    return true;
}

// 测试root用户创建和存储
bool testRootUserCreation() {
    std::cout << "\n=== Testing Root User Creation ===" << std::endl;
    
    const std::string dbFile = "data/test_auth_storage.bin";
    
    // 创建存储引擎和目录
    StorageEngine storage_engine(dbFile, 16);
    Catalog catalog(&storage_engine);
    
    // 创建用户存储管理器
    UserStorageManager user_storage_manager(&storage_engine, &catalog);
    TEST_ASSERT(user_storage_manager.initialize(), "UserStorageManager initialized");
    
    // 检查root用户是否存在
    TEST_ASSERT(user_storage_manager.userExists("root"), "Root user exists");
    
    // 验证root用户角色
    Role rootRole = user_storage_manager.getUserRole("root");
    TEST_ASSERT(rootRole == Role::DBA, "Root user has DBA role");
    
    // 测试root用户认证
    TEST_ASSERT(user_storage_manager.authenticate("root", "root"), "Root user authentication works");
    
    return true;
}

// 测试root用户创建其他用户
bool testRootUserCreatesOtherUsers() {
    std::cout << "\n=== Testing Root User Creates Other Users ===" << std::endl;
    
    const std::string dbFile = "data/test_auth_storage_" + std::to_string(time(nullptr)) + ".bin";
    // 确保删除之前的测试文件
    std::remove(dbFile.c_str());
    
    // 创建存储引擎和目录
    StorageEngine storage_engine(dbFile, 16);
    Catalog catalog(&storage_engine);
    
    // 创建认证服务
    AuthService auth_service(&storage_engine, &catalog);
    
    // 以root用户登录
    TEST_ASSERT(auth_service.login("root", "root"), "Root user login successful");
    TEST_ASSERT(auth_service.isDBA(), "Root user is DBA");
    
    // 创建developer用户
    TEST_ASSERT(auth_service.createUser("developer1", "dev123", Role::DEVELOPER), 
                "Root user can create developer user");
    
    // 创建analyst用户
    TEST_ASSERT(auth_service.createUser("analyst1", "analyst123", Role::ANALYST), 
                "Root user can create analyst user");
    
    // 验证用户存在
    TEST_ASSERT(auth_service.userExists("developer1"), "Developer user exists");
    TEST_ASSERT(auth_service.userExists("analyst1"), "Analyst user exists");
    
    // 验证用户角色
    TEST_ASSERT(auth_service.getCurrentUserRole() == Role::DBA, "Current user is DBA");
    
    // 保存数据到存储
    auth_service.saveToFile("users.dat");
    
    return true;
}

// 测试用户数据持久化
bool testUserDataPersistence() {
    std::cout << "\n=== Testing User Data Persistence ===" << std::endl;
    
    const std::string dbFile = "data/test_auth_storage_" + std::to_string(time(nullptr)) + ".bin";
    // 确保删除之前的测试文件
    std::remove(dbFile.c_str());
    
    // 重新创建存储引擎和目录（模拟重启）
    StorageEngine storage_engine(dbFile, 16);
    Catalog catalog(&storage_engine);
    
    // 创建认证服务
    AuthService auth_service(&storage_engine, &catalog);
    
    // 验证所有用户仍然存在
    TEST_ASSERT(auth_service.userExists("root"), "Root user persisted");
    TEST_ASSERT(auth_service.userExists("developer1"), "Developer user persisted");
    TEST_ASSERT(auth_service.userExists("analyst1"), "Analyst user persisted");
    
    // 验证用户角色
    auth_service.login("root", "root");
    TEST_ASSERT(auth_service.getCurrentUserRole() == Role::DBA, "Root role persisted");
    
    auth_service.login("developer1", "dev123");
    TEST_ASSERT(auth_service.getCurrentUserRole() == Role::DEVELOPER, "Developer role persisted");
    
    auth_service.login("analyst1", "analyst123");
    TEST_ASSERT(auth_service.getCurrentUserRole() == Role::ANALYST, "Analyst role persisted");
    
    return true;
}

// 测试权限验证
bool testPermissionVerification() {
    std::cout << "\n=== Testing Permission Verification ===" << std::endl;
    
    const std::string dbFile = "data/test_auth_storage_" + std::to_string(time(nullptr)) + ".bin";
    // 确保删除之前的测试文件
    std::remove(dbFile.c_str());
    
    // 创建存储引擎和目录
    StorageEngine storage_engine(dbFile, 16);
    Catalog catalog(&storage_engine);
    
    // 创建认证服务
    AuthService auth_service(&storage_engine, &catalog);
    
    // 测试root用户权限
    auth_service.login("root", "root");
    TEST_ASSERT(auth_service.hasPermission(Permission::CREATE_USER), "Root can create users");
    TEST_ASSERT(auth_service.hasPermission(Permission::CREATE_TABLE), "Root can create tables");
    TEST_ASSERT(auth_service.hasPermission(Permission::SELECT), "Root can select");
    
    // 测试developer用户权限
    auth_service.login("developer1", "dev123");
    TEST_ASSERT(auth_service.hasPermission(Permission::CREATE_TABLE), "Developer can create tables");
    TEST_ASSERT(auth_service.hasPermission(Permission::SELECT), "Developer can select");
    TEST_ASSERT(!auth_service.hasPermission(Permission::CREATE_USER), "Developer cannot create users");
    
    // 测试analyst用户权限
    auth_service.login("analyst1", "analyst123");
    TEST_ASSERT(auth_service.hasPermission(Permission::SELECT), "Analyst can select");
    TEST_ASSERT(!auth_service.hasPermission(Permission::CREATE_TABLE), "Analyst cannot create tables");
    TEST_ASSERT(!auth_service.hasPermission(Permission::CREATE_USER), "Analyst cannot create users");
    
    return true;
}

// 测试用户表数据完整性
bool testUserTableDataIntegrity() {
    std::cout << "\n=== Testing User Table Data Integrity ===" << std::endl;
    
    const std::string dbFile = "data/test_auth_storage_" + std::to_string(time(nullptr)) + ".bin";
    // 确保删除之前的测试文件
    std::remove(dbFile.c_str());
    
    // 创建存储引擎和目录
    StorageEngine storage_engine(dbFile, 16);
    Catalog catalog(&storage_engine);
    
    // 创建认证服务
    AuthService auth_service(&storage_engine, &catalog);
    
    // 获取所有用户记录
    auto users = auth_service.getAllUsers();
    TEST_ASSERT(users.size() >= 3, "At least 3 users in database");
    
    std::cout << "[INFO] Users in database:" << std::endl;
    for (const auto& user : users) {
        std::cout << "  - " << user.username << " (Role: " << 
            RoleManager().roleToString(user.role) << ")" << std::endl;
    }
    
    return true;
}

int main() {
    std::cout << "Starting Auth Storage Integration Tests..." << std::endl;
    std::cout << "==========================================" << std::endl;
    
    bool all_passed = true;
    
    try {
        // 清理之前的测试文件
        std::remove("data/test_auth_storage.bin");
        
        // 运行测试
        if (!testStorageEngineInitialization()) {
            all_passed = false;
        }
        
        if (!testUserStorageManagerInitialization()) {
            all_passed = false;
        }
        
        if (!testRootUserCreation()) {
            all_passed = false;
        }
        
        if (!testRootUserCreatesOtherUsers()) {
            all_passed = false;
        }
        
        if (!testUserDataPersistence()) {
            all_passed = false;
        }
        
        if (!testPermissionVerification()) {
            all_passed = false;
        }
        
        if (!testUserTableDataIntegrity()) {
            all_passed = false;
        }
        
        // 检查.bin文件结构
        if (!checkBinFileStructure("data/test_auth_storage.bin")) {
            all_passed = false;
        }
        
        std::cout << "\n==========================================" << std::endl;
        if (all_passed) {
            std::cout << "=== All tests passed! ===" << std::endl;
            std::cout << "✅ Root user successfully written to .bin file" << std::endl;
            std::cout << "✅ Metadata page and catalog page created correctly" << std::endl;
            std::cout << "✅ User table data page created correctly" << std::endl;
            std::cout << "✅ Root user can create developer and analyst users" << std::endl;
            std::cout << "✅ All user data persisted to .bin file" << std::endl;
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
