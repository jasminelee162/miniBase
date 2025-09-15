#include "../../src/auth/auth_service.h"
#include "../../src/storage/storage_engine.h"
#include "../../src/catalog/catalog.h"
#include <iostream>
#include <string>
#include <vector>
#include <ctime>
#include <cstdio>

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

int main() {
    std::cout << "=== 调试权限检查问题 ===" << std::endl;
    
    const std::string dbFile = "data/debug_permissions.bin";
    std::remove(dbFile.c_str());

    StorageEngine storage_engine(dbFile, 16);
    Catalog catalog(&storage_engine);
    AuthService auth_service(&storage_engine, &catalog);

    // 1. 测试root用户登录
    std::cout << "\n1. 测试root用户登录" << std::endl;
    bool login_result = auth_service.login("root", "root");
    std::cout << "登录结果: " << (login_result ? "成功" : "失败") << std::endl;
    TEST_ASSERT(login_result, "root用户登录应该成功");
    
    // 2. 检查当前用户和角色
    std::cout << "\n2. 检查当前用户和角色" << std::endl;
    std::string current_user = auth_service.getCurrentUser();
    Role current_role = auth_service.getCurrentUserRole();
    std::cout << "当前用户: " << current_user << std::endl;
    std::cout << "当前角色: " << auth_service.getCurrentUserRoleString() << std::endl;
    
    // 3. 检查基本权限
    std::cout << "\n3. 检查基本权限" << std::endl;
    bool has_insert = auth_service.hasPermission(Permission::INSERT);
    bool has_select = auth_service.hasPermission(Permission::SELECT);
    std::cout << "INSERT权限: " << (has_insert ? "有" : "无") << std::endl;
    std::cout << "SELECT权限: " << (has_select ? "有" : "无") << std::endl;
    
    // 4. 创建表
    std::cout << "\n4. 创建表" << std::endl;
    std::vector<Column> columns = {{"id", "INT", -1}, {"name", "VARCHAR", 50}};
    catalog.CreateTable("test_table", columns, "root");
    std::cout << "表创建完成" << std::endl;
    
    // 5. 检查表权限
    std::cout << "\n5. 检查表权限" << std::endl;
    bool table_select = auth_service.checkTablePermission("test_table", Permission::SELECT);
    bool table_insert = auth_service.checkTablePermission("test_table", Permission::INSERT);
    std::cout << "表SELECT权限: " << (table_select ? "有" : "无") << std::endl;
    std::cout << "表INSERT权限: " << (table_insert ? "有" : "无") << std::endl;
    
    // 6. 检查表所有者
    std::cout << "\n6. 检查表所有者" << std::endl;
    std::string table_owner = catalog.GetTableOwner("test_table");
    bool is_owner = catalog.IsTableOwner("test_table", "root");
    std::cout << "表所有者: " << table_owner << std::endl;
    std::cout << "root是所有者: " << (is_owner ? "是" : "否") << std::endl;
    
    // 7. 测试创建DEVELOPER用户
    std::cout << "\n7. 测试创建DEVELOPER用户" << std::endl;
    bool create_user = auth_service.createUser("devuser", "devpass", Role::DEVELOPER);
    std::cout << "创建用户结果: " << (create_user ? "成功" : "失败") << std::endl;
    TEST_ASSERT(create_user, "创建DEVELOPER用户应该成功");
    
    // 8. 登出并登录devuser
    std::cout << "\n8. 登出并登录devuser" << std::endl;
    auth_service.logout();
    bool dev_login = auth_service.login("devuser", "devpass");
    std::cout << "devuser登录结果: " << (dev_login ? "成功" : "失败") << std::endl;
    TEST_ASSERT(dev_login, "devuser登录应该成功");
    
    // 9. 检查devuser的权限
    std::cout << "\n9. 检查devuser的权限" << std::endl;
    std::string dev_user = auth_service.getCurrentUser();
    Role dev_role = auth_service.getCurrentUserRole();
    std::cout << "devuser: " << dev_user << std::endl;
    std::cout << "devuser角色: " << auth_service.getCurrentUserRoleString() << std::endl;
    
    bool dev_has_insert = auth_service.hasPermission(Permission::INSERT);
    std::cout << "devuser INSERT权限: " << (dev_has_insert ? "有" : "无") << std::endl;
    
    // 10. 检查devuser对表的权限
    std::cout << "\n10. 检查devuser对表的权限" << std::endl;
    bool dev_table_select = auth_service.checkTablePermission("test_table", Permission::SELECT);
    bool dev_table_insert = auth_service.checkTablePermission("test_table", Permission::INSERT);
    std::cout << "devuser表SELECT权限: " << (dev_table_select ? "有" : "无") << std::endl;
    std::cout << "devuser表INSERT权限: " << (dev_table_insert ? "有" : "无") << std::endl;
    
    std::cout << "\n=== 调试完成 ===" << std::endl;
    
    cleanupTestFiles(dbFile);
    return 0;
}
