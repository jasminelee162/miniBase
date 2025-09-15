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

// 测试DBA用户权限
bool testDBATablePermissions() {
    std::cout << "\n=== 测试DBA用户表权限 ===" << std::endl;
    const std::string dbFile = "data/test_dba_permissions_" + std::to_string(time(nullptr)) + ".bin";
    cleanupTestFiles(dbFile);

    StorageEngine storage_engine(dbFile, 16);
    Catalog catalog(&storage_engine);
    AuthService auth_service(&storage_engine, &catalog);

    // DBA登录
    TEST_ASSERT(auth_service.login("root", "root"), "DBA用户登录应该成功");
    
    // 创建表（应该成功）
    std::vector<Column> columns = {{"id", "INT", -1}, {"name", "VARCHAR", 50}};
    catalog.CreateTable("test_table", columns, "root");
    
    // DBA应该可以操作所有表
    TEST_ASSERT(auth_service.checkTablePermission("test_table", Permission::SELECT), "DBA应该可以查询所有表");
    TEST_ASSERT(auth_service.checkTablePermission("test_table", Permission::INSERT), "DBA应该可以插入到所有表");
    TEST_ASSERT(auth_service.checkTablePermission("test_table", Permission::UPDATE), "DBA应该可以更新所有表");
    TEST_ASSERT(auth_service.checkTablePermission("test_table", Permission::DELETE), "DBA应该可以删除所有表");
    TEST_ASSERT(auth_service.checkTablePermission("test_table", Permission::CREATE_TABLE), "DBA应该可以创建表");
    TEST_ASSERT(auth_service.checkTablePermission("test_table", Permission::DROP_TABLE), "DBA应该可以删除表");

    cleanupTestFiles(dbFile);
    return true;
}

// 测试DEVELOPER用户权限
bool testDeveloperTablePermissions() {
    std::cout << "\n=== 测试DEVELOPER用户表权限 ===" << std::endl;
    const std::string dbFile = "data/test_dev_permissions_" + std::to_string(time(nullptr)) + ".bin";
    cleanupTestFiles(dbFile);

    StorageEngine storage_engine(dbFile, 16);
    Catalog catalog(&storage_engine);
    AuthService auth_service(&storage_engine, &catalog);

    // 创建DEVELOPER用户
    TEST_ASSERT(auth_service.login("root", "root"), "root用户登录应该成功");
    TEST_ASSERT(auth_service.createUser("devuser", "devpass", Role::DEVELOPER), "创建DEVELOPER用户应该成功");
    
    // 创建两个表：一个属于root，一个属于devuser
    std::vector<Column> columns = {{"id", "INT", -1}, {"name", "VARCHAR", 50}};
    catalog.CreateTable("root_table", columns, "root");
    catalog.CreateTable("dev_table", columns, "devuser");
    
    // 登出root，登录devuser
    auth_service.logout();
    TEST_ASSERT(auth_service.login("devuser", "devpass"), "devuser登录应该成功");
    
    // DEVELOPER应该只能操作自己创建的表
    TEST_ASSERT(auth_service.checkTablePermission("dev_table", Permission::SELECT), "DEVELOPER应该可以查询自己创建的表");
    TEST_ASSERT(auth_service.checkTablePermission("dev_table", Permission::INSERT), "DEVELOPER应该可以插入到自己创建的表");
    TEST_ASSERT(auth_service.checkTablePermission("dev_table", Permission::UPDATE), "DEVELOPER应该可以更新自己创建的表");
    TEST_ASSERT(auth_service.checkTablePermission("dev_table", Permission::DELETE), "DEVELOPER应该可以删除自己创建的表");
    
    // 不能操作其他用户的表
    TEST_ASSERT(!auth_service.checkTablePermission("root_table", Permission::SELECT), "DEVELOPER不应该可以查询其他用户的表");
    TEST_ASSERT(!auth_service.checkTablePermission("root_table", Permission::INSERT), "DEVELOPER不应该可以插入到其他用户的表");
    TEST_ASSERT(!auth_service.checkTablePermission("root_table", Permission::UPDATE), "DEVELOPER不应该可以更新其他用户的表");
    TEST_ASSERT(!auth_service.checkTablePermission("root_table", Permission::DELETE), "DEVELOPER不应该可以删除其他用户的表");

    cleanupTestFiles(dbFile);
    return true;
}

// 测试ANALYST用户权限
bool testAnalystTablePermissions() {
    std::cout << "\n=== 测试ANALYST用户表权限 ===" << std::endl;
    const std::string dbFile = "data/test_analyst_permissions_" + std::to_string(time(nullptr)) + ".bin";
    cleanupTestFiles(dbFile);

    StorageEngine storage_engine(dbFile, 16);
    Catalog catalog(&storage_engine);
    AuthService auth_service(&storage_engine, &catalog);

    // 创建ANALYST用户
    TEST_ASSERT(auth_service.login("root", "root"), "root用户登录应该成功");
    TEST_ASSERT(auth_service.createUser("analystuser", "analystpass", Role::ANALYST), "创建ANALYST用户应该成功");
    
    // 创建表
    std::vector<Column> columns = {{"id", "INT", -1}, {"name", "VARCHAR", 50}};
    catalog.CreateTable("test_table", columns, "root");
    
    // 登出root，登录analystuser
    auth_service.logout();
    TEST_ASSERT(auth_service.login("analystuser", "analystpass"), "analystuser登录应该成功");
    
    // ANALYST只能查看所有表
    TEST_ASSERT(auth_service.checkTablePermission("test_table", Permission::SELECT), "ANALYST应该可以查询所有表");
    
    // 不能进行写操作
    TEST_ASSERT(!auth_service.checkTablePermission("test_table", Permission::INSERT), "ANALYST不应该可以插入数据");
    TEST_ASSERT(!auth_service.checkTablePermission("test_table", Permission::UPDATE), "ANALYST不应该可以更新数据");
    TEST_ASSERT(!auth_service.checkTablePermission("test_table", Permission::DELETE), "ANALYST不应该可以删除数据");
    TEST_ASSERT(!auth_service.checkTablePermission("test_table", Permission::CREATE_TABLE), "ANALYST不应该可以创建表");
    TEST_ASSERT(!auth_service.checkTablePermission("test_table", Permission::DROP_TABLE), "ANALYST不应该可以删除表");

    cleanupTestFiles(dbFile);
    return true;
}

// 测试表所有者信息
bool testTableOwnership() {
    std::cout << "\n=== 测试表所有者信息 ===" << std::endl;
    const std::string dbFile = "data/test_ownership_" + std::to_string(time(nullptr)) + ".bin";
    cleanupTestFiles(dbFile);

    StorageEngine storage_engine(dbFile, 16);
    Catalog catalog(&storage_engine);
    AuthService auth_service(&storage_engine, &catalog);

    // 创建用户
    TEST_ASSERT(auth_service.login("root", "root"), "root用户登录应该成功");
    TEST_ASSERT(auth_service.createUser("user1", "pass1", Role::DEVELOPER), "创建user1应该成功");
    TEST_ASSERT(auth_service.createUser("user2", "pass2", Role::DEVELOPER), "创建user2应该成功");
    
    // 创建表
    std::vector<Column> columns = {{"id", "INT", -1}, {"name", "VARCHAR", 50}};
    catalog.CreateTable("table1", columns, "user1");
    catalog.CreateTable("table2", columns, "user2");
    catalog.CreateTable("system_table", columns, ""); // 系统表
    
    // 测试表所有者信息
    TEST_ASSERT(catalog.GetTableOwner("table1") == "user1", "table1的所有者应该是user1");
    TEST_ASSERT(catalog.GetTableOwner("table2") == "user2", "table2的所有者应该是user2");
    TEST_ASSERT(catalog.GetTableOwner("system_table") == "", "system_table的所有者应该为空");
    
    // 测试所有权检查
    TEST_ASSERT(catalog.IsTableOwner("table1", "user1"), "user1应该是table1的所有者");
    TEST_ASSERT(catalog.IsTableOwner("table2", "user2"), "user2应该是table2的所有者");
    TEST_ASSERT(!catalog.IsTableOwner("table1", "user2"), "user2不应该是table1的所有者");
    TEST_ASSERT(!catalog.IsTableOwner("table2", "user1"), "user1不应该是table2的所有者");
    
    // 测试按所有者获取表列表
    auto user1_tables = catalog.GetTablesByOwner("user1");
    TEST_ASSERT(user1_tables.size() == 1, "user1应该只有1张表");
    TEST_ASSERT(user1_tables[0] == "table1", "user1的表应该是table1");
    
    auto user2_tables = catalog.GetTablesByOwner("user2");
    TEST_ASSERT(user2_tables.size() == 1, "user2应该只有1张表");
    TEST_ASSERT(user2_tables[0] == "table2", "user2的表应该是table2");

    cleanupTestFiles(dbFile);
    return true;
}

int main() {
    std::cout << "=== 表级别权限控制测试程序 ===" << std::endl;
    std::cout << "测试基于表所有者的权限控制机制" << std::endl;
    std::cout << "===============================================" << std::endl;

    bool all_passed = true;

    try {
        all_passed &= testDBATablePermissions();
        all_passed &= testDeveloperTablePermissions();
        all_passed &= testAnalystTablePermissions();
        all_passed &= testTableOwnership();
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] 测试过程中发生异常: " << e.what() << std::endl;
        all_passed = false;
    }

    std::cout << "\n===============================================" << std::endl;
    if (all_passed) {
        std::cout << "✅ 所有表级别权限测试通过！" << std::endl;
    } else {
        std::cout << "❌ 部分表级别权限测试失败，需要修复问题" << std::endl;
    }

    return all_passed ? 0 : 1;
}
