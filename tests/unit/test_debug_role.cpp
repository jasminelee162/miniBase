#include "../../src/auth/auth_service.h"
#include "../../src/storage/storage_engine.h"
#include "../../src/catalog/catalog.h"
#include <iostream>
#include <string>
#include <vector>
#include <ctime>
#include <cstdio>

using namespace minidb;

int main() {
    std::cout << "=== 调试角色获取问题 ===" << std::endl;
    
    const std::string dbFile = "data/debug_role.bin";
    std::remove(dbFile.c_str());

    StorageEngine storage_engine(dbFile, 16);
    Catalog catalog(&storage_engine);
    AuthService auth_service(&storage_engine, &catalog);

    // 1. 测试root用户登录
    std::cout << "\n1. 测试root用户登录" << std::endl;
    bool login_result = auth_service.login("root", "root");
    std::cout << "登录结果: " << (login_result ? "成功" : "失败") << std::endl;
    
    if (login_result) {
        // 2. 检查当前用户
        std::cout << "\n2. 检查当前用户" << std::endl;
        std::string current_user = auth_service.getCurrentUser();
        std::cout << "当前用户: " << current_user << std::endl;
        
        // 3. 检查角色获取
        std::cout << "\n3. 检查角色获取" << std::endl;
        Role current_role = auth_service.getCurrentUserRole();
        std::cout << "当前角色枚举值: " << static_cast<int>(current_role) << std::endl;
        std::cout << "当前角色字符串: " << auth_service.getCurrentUserRoleString() << std::endl;
        
        // 4. 检查DBA判断
        std::cout << "\n4. 检查DBA判断" << std::endl;
        bool is_dba = auth_service.isDBA();
        std::cout << "是否为DBA: " << (is_dba ? "是" : "否") << std::endl;
        
        // 5. 检查权限
        std::cout << "\n5. 检查权限" << std::endl;
        bool has_insert = auth_service.hasPermission(Permission::INSERT);
        bool has_select = auth_service.hasPermission(Permission::SELECT);
        bool has_create_user = auth_service.hasPermission(Permission::CREATE_USER);
        std::cout << "INSERT权限: " << (has_insert ? "有" : "无") << std::endl;
        std::cout << "SELECT权限: " << (has_select ? "有" : "无") << std::endl;
        std::cout << "CREATE_USER权限: " << (has_create_user ? "有" : "无") << std::endl;
        
        // 6. 直接测试RoleManager
        std::cout << "\n6. 直接测试RoleManager" << std::endl;
        RoleManager role_manager;
        bool rm_has_insert = role_manager.hasPermission(Role::DBA, Permission::INSERT);
        bool rm_has_select = role_manager.hasPermission(Role::DBA, Permission::SELECT);
        bool rm_has_create_user = role_manager.hasPermission(Role::DBA, Permission::CREATE_USER);
        std::cout << "RoleManager DBA INSERT权限: " << (rm_has_insert ? "有" : "无") << std::endl;
        std::cout << "RoleManager DBA SELECT权限: " << (rm_has_select ? "有" : "无") << std::endl;
        std::cout << "RoleManager DBA CREATE_USER权限: " << (rm_has_create_user ? "有" : "无") << std::endl;
        
        // 7. 检查用户存储管理器
        std::cout << "\n7. 检查用户存储管理器" << std::endl;
        // 这里需要访问私有成员，我们通过公共接口间接测试
        bool user_exists = auth_service.userExists("root");
        std::cout << "root用户存在: " << (user_exists ? "是" : "否") << std::endl;
    }
    
    std::cout << "\n=== 调试完成 ===" << std::endl;
    
    std::remove(dbFile.c_str());
    return 0;
}
