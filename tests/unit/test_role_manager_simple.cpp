#include "../../src/auth/role_manager.h"
#include <iostream>

using namespace minidb;

int main() {
    std::cout << "=== 测试RoleManager简单功能 ===" << std::endl;
    
    RoleManager role_manager;
    
    // 1. 测试DBA权限
    std::cout << "\n1. 测试DBA权限" << std::endl;
    bool has_insert = role_manager.hasPermission(Role::DBA, Permission::INSERT);
    bool has_select = role_manager.hasPermission(Role::DBA, Permission::SELECT);
    bool has_create_user = role_manager.hasPermission(Role::DBA, Permission::CREATE_USER);
    
    std::cout << "DBA INSERT权限: " << (has_insert ? "有" : "无") << std::endl;
    std::cout << "DBA SELECT权限: " << (has_select ? "有" : "无") << std::endl;
    std::cout << "DBA CREATE_USER权限: " << (has_create_user ? "有" : "无") << std::endl;
    
    // 2. 测试DEVELOPER权限
    std::cout << "\n2. 测试DEVELOPER权限" << std::endl;
    bool dev_has_insert = role_manager.hasPermission(Role::DEVELOPER, Permission::INSERT);
    bool dev_has_select = role_manager.hasPermission(Role::DEVELOPER, Permission::SELECT);
    bool dev_has_create_user = role_manager.hasPermission(Role::DEVELOPER, Permission::CREATE_USER);
    
    std::cout << "DEVELOPER INSERT权限: " << (dev_has_insert ? "有" : "无") << std::endl;
    std::cout << "DEVELOPER SELECT权限: " << (dev_has_select ? "有" : "无") << std::endl;
    std::cout << "DEVELOPER CREATE_USER权限: " << (dev_has_create_user ? "有" : "无") << std::endl;
    
    // 3. 测试ANALYST权限
    std::cout << "\n3. 测试ANALYST权限" << std::endl;
    bool analyst_has_insert = role_manager.hasPermission(Role::ANALYST, Permission::INSERT);
    bool analyst_has_select = role_manager.hasPermission(Role::ANALYST, Permission::SELECT);
    bool analyst_has_create_user = role_manager.hasPermission(Role::ANALYST, Permission::CREATE_USER);
    
    std::cout << "ANALYST INSERT权限: " << (analyst_has_insert ? "有" : "无") << std::endl;
    std::cout << "ANALYST SELECT权限: " << (analyst_has_select ? "有" : "无") << std::endl;
    std::cout << "ANALYST CREATE_USER权限: " << (analyst_has_create_user ? "有" : "无") << std::endl;
    
    // 4. 获取权限列表
    std::cout << "\n4. 获取权限列表" << std::endl;
    auto dba_permissions = role_manager.getRolePermissions(Role::DBA);
    std::cout << "DBA权限数量: " << dba_permissions.size() << std::endl;
    
    auto dev_permissions = role_manager.getRolePermissions(Role::DEVELOPER);
    std::cout << "DEVELOPER权限数量: " << dev_permissions.size() << std::endl;
    
    auto analyst_permissions = role_manager.getRolePermissions(Role::ANALYST);
    std::cout << "ANALYST权限数量: " << analyst_permissions.size() << std::endl;
    
    std::cout << "\n=== 测试完成 ===" << std::endl;
    return 0;
}
