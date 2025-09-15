#include "../../src/auth/role_manager.h"
#include <iostream>
#include <set>

using namespace minidb;

int main() {
    std::cout << "=== 调试std::set问题 ===" << std::endl;
    
    RoleManager role_manager;
    
    // 1. 检查DBA角色权限集合
    std::cout << "\n1. 检查DBA角色权限集合" << std::endl;
    auto dba_permissions = role_manager.getRolePermissions(Role::DBA);
    std::cout << "DBA权限数量: " << dba_permissions.size() << std::endl;
    
    for (const auto& perm : dba_permissions) {
        std::cout << "权限: " << role_manager.permissionToString(perm) << std::endl;
    }
    
    // 2. 直接测试权限检查
    std::cout << "\n2. 直接测试权限检查" << std::endl;
    bool has_insert = role_manager.hasPermission(Role::DBA, Permission::INSERT);
    bool has_select = role_manager.hasPermission(Role::DBA, Permission::SELECT);
    bool has_create_user = role_manager.hasPermission(Role::DBA, Permission::CREATE_USER);
    
    std::cout << "DBA INSERT权限: " << (has_insert ? "有" : "无") << std::endl;
    std::cout << "DBA SELECT权限: " << (has_select ? "有" : "无") << std::endl;
    std::cout << "DBA CREATE_USER权限: " << (has_create_user ? "有" : "无") << std::endl;
    
    // 3. 测试枚举值
    std::cout << "\n3. 测试枚举值" << std::endl;
    std::cout << "Role::DBA值: " << static_cast<int>(Role::DBA) << std::endl;
    std::cout << "Permission::INSERT值: " << static_cast<int>(Permission::INSERT) << std::endl;
    std::cout << "Permission::SELECT值: " << static_cast<int>(Permission::SELECT) << std::endl;
    std::cout << "Permission::CREATE_USER值: " << static_cast<int>(Permission::CREATE_USER) << std::endl;
    
    // 4. 手动创建set测试
    std::cout << "\n4. 手动创建set测试" << std::endl;
    std::set<Permission> test_set = {Permission::INSERT, Permission::SELECT, Permission::CREATE_USER};
    std::cout << "手动set中INSERT: " << (test_set.count(Permission::INSERT) > 0 ? "有" : "无") << std::endl;
    std::cout << "手动set中SELECT: " << (test_set.count(Permission::SELECT) > 0 ? "有" : "无") << std::endl;
    std::cout << "手动set中CREATE_USER: " << (test_set.count(Permission::CREATE_USER) > 0 ? "有" : "无") << std::endl;
    
    // 5. 测试所有权限
    std::cout << "\n5. 测试所有权限" << std::endl;
    std::vector<Permission> all_permissions = {
        Permission::CREATE_TABLE, Permission::DROP_TABLE, Permission::ALTER_TABLE,
        Permission::CREATE_INDEX, Permission::DROP_INDEX,
        Permission::SELECT, Permission::INSERT, Permission::UPDATE, Permission::DELETE,
        Permission::CREATE_USER, Permission::DROP_USER, Permission::GRANT, Permission::REVOKE,
        Permission::SHOW_PROCESSES, Permission::KILL_PROCESS,
        Permission::SHOW_VARIABLES, Permission::SET_VARIABLES
    };
    
    for (const auto& perm : all_permissions) {
        bool has_perm = role_manager.hasPermission(Role::DBA, perm);
        std::cout << role_manager.permissionToString(perm) << ": " << (has_perm ? "有" : "无") << std::endl;
    }
    
    std::cout << "\n=== 调试完成 ===" << std::endl;
    return 0;
}
