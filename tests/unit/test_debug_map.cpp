#include "../../src/auth/role_manager.h"
#include <iostream>
#include <unordered_map>
#include <set>

using namespace minidb;

int main() {
    std::cout << "=== 调试unordered_map问题 ===" << std::endl;
    
    RoleManager role_manager;
    
    // 1. 检查role_permissions_映射
    std::cout << "\n1. 检查role_permissions_映射" << std::endl;
    
    // 手动创建映射测试
    std::unordered_map<Role, std::set<Permission>> test_map;
    test_map[Role::DBA] = {Permission::INSERT, Permission::SELECT, Permission::CREATE_USER};
    
    std::cout << "手动映射中DBA权限数量: " << test_map[Role::DBA].size() << std::endl;
    std::cout << "手动映射中DBA INSERT权限: " << (test_map[Role::DBA].count(Permission::INSERT) > 0 ? "有" : "无") << std::endl;
    std::cout << "手动映射中DBA SELECT权限: " << (test_map[Role::DBA].count(Permission::SELECT) > 0 ? "有" : "无") << std::endl;
    std::cout << "手动映射中DBA CREATE_USER权限: " << (test_map[Role::DBA].count(Permission::CREATE_USER) > 0 ? "有" : "无") << std::endl;
    
    // 2. 测试RoleManager的权限检查
    std::cout << "\n2. 测试RoleManager的权限检查" << std::endl;
    bool has_insert = role_manager.hasPermission(Role::DBA, Permission::INSERT);
    bool has_select = role_manager.hasPermission(Role::DBA, Permission::SELECT);
    bool has_create_user = role_manager.hasPermission(Role::DBA, Permission::CREATE_USER);
    
    std::cout << "RoleManager DBA INSERT权限: " << (has_insert ? "有" : "无") << std::endl;
    std::cout << "RoleManager DBA SELECT权限: " << (has_select ? "有" : "无") << std::endl;
    std::cout << "RoleManager DBA CREATE_USER权限: " << (has_create_user ? "有" : "无") << std::endl;
    
    // 3. 测试Role枚举值
    std::cout << "\n3. 测试Role枚举值" << std::endl;
    std::cout << "Role::DBA值: " << static_cast<int>(Role::DBA) << std::endl;
    std::cout << "Role::DEVELOPER值: " << static_cast<int>(Role::DEVELOPER) << std::endl;
    std::cout << "Role::ANALYST值: " << static_cast<int>(Role::ANALYST) << std::endl;
    
    // 4. 测试Role比较操作符
    std::cout << "\n4. 测试Role比较操作符" << std::endl;
    std::cout << "DBA < DEVELOPER: " << (Role::DBA < Role::DEVELOPER) << std::endl;
    std::cout << "DEVELOPER < ANALYST: " << (Role::DEVELOPER < Role::ANALYST) << std::endl;
    std::cout << "DBA < ANALYST: " << (Role::DBA < Role::ANALYST) << std::endl;
    
    // 5. 测试unordered_map的find方法
    std::cout << "\n5. 测试unordered_map的find方法" << std::endl;
    auto it = test_map.find(Role::DBA);
    if (it != test_map.end()) {
        std::cout << "找到DBA角色，权限数量: " << it->second.size() << std::endl;
        std::cout << "DBA INSERT权限: " << (it->second.count(Permission::INSERT) > 0 ? "有" : "无") << std::endl;
    } else {
        std::cout << "未找到DBA角色" << std::endl;
    }
    
    std::cout << "\n=== 调试完成 ===" << std::endl;
    return 0;
}
