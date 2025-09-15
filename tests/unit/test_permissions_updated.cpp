#include "../../src/auth/role_manager.h"
#include "../../src/auth/auth_service.h"
#include <iostream>
#include <cassert>

using namespace minidb;

void test_updated_permissions() {
    std::cout << "=== Testing Updated Permissions ===" << std::endl;
    
    RoleManager role_manager;
    
    // 测试DBA权限
    std::cout << "\nDBA Permissions:" << std::endl;
    auto dba_permissions = role_manager.getRolePermissions(Role::DBA);
    for (const auto& perm : dba_permissions) {
        std::cout << "  - " << role_manager.permissionToString(perm) << std::endl;
    }
    
    // 测试DEVELOPER权限
    std::cout << "\nDEVELOPER Permissions:" << std::endl;
    auto dev_permissions = role_manager.getRolePermissions(Role::DEVELOPER);
    for (const auto& perm : dev_permissions) {
        std::cout << "  - " << role_manager.permissionToString(perm) << std::endl;
    }
    
    // 测试ANALYST权限
    std::cout << "\nANALYST Permissions:" << std::endl;
    auto analyst_permissions = role_manager.getRolePermissions(Role::ANALYST);
    for (const auto& perm : analyst_permissions) {
        std::cout << "  - " << role_manager.permissionToString(perm) << std::endl;
    }
    
    // 测试特定权限检查
    std::cout << "\nPermission Checks:" << std::endl;
    std::cout << "DBA can CREATE_TABLE: " << (role_manager.hasPermission(Role::DBA, Permission::CREATE_TABLE) ? "Yes" : "No") << std::endl;
    std::cout << "DBA can CREATE_USER: " << (role_manager.hasPermission(Role::DBA, Permission::CREATE_USER) ? "Yes" : "No") << std::endl;
    std::cout << "DBA can GRANT: " << (role_manager.hasPermission(Role::DBA, Permission::GRANT) ? "Yes" : "No") << std::endl;
    std::cout << "DEVELOPER can CREATE_USER: " << (role_manager.hasPermission(Role::DEVELOPER, Permission::CREATE_USER) ? "Yes" : "No") << std::endl;
    std::cout << "ANALYST can SELECT: " << (role_manager.hasPermission(Role::ANALYST, Permission::SELECT) ? "Yes" : "No") << std::endl;
    std::cout << "ANALYST can INSERT: " << (role_manager.hasPermission(Role::ANALYST, Permission::INSERT) ? "Yes" : "No") << std::endl;
    
    // 验证权限数量
    std::cout << "\nPermission Counts:" << std::endl;
    std::cout << "DBA permissions: " << dba_permissions.size() << std::endl;
    std::cout << "DEVELOPER permissions: " << dev_permissions.size() << std::endl;
    std::cout << "ANALYST permissions: " << analyst_permissions.size() << std::endl;
    
    // 验证权限正确性
    assert(role_manager.hasPermission(Role::DBA, Permission::CREATE_USER));
    assert(role_manager.hasPermission(Role::DBA, Permission::CREATE_TABLE));
    assert(role_manager.hasPermission(Role::DBA, Permission::SELECT));
    assert(!role_manager.hasPermission(Role::DEVELOPER, Permission::CREATE_USER));
    assert(role_manager.hasPermission(Role::DEVELOPER, Permission::CREATE_TABLE));
    assert(role_manager.hasPermission(Role::ANALYST, Permission::SELECT));
    assert(!role_manager.hasPermission(Role::ANALYST, Permission::INSERT));
    
    std::cout << "\n✅ All permission tests passed!" << std::endl;
}

void test_auth_service_with_updated_permissions() {
    std::cout << "\n=== Testing AuthService with Updated Permissions ===" << std::endl;
    
    AuthService auth_service;
    
    // 测试登录
    if (auth_service.login("admin", "admin123")) {
        std::cout << "Login successful!" << std::endl;
        std::cout << "Current user: " << auth_service.getCurrentUser() << std::endl;
        std::cout << "Role: " << auth_service.getCurrentUserRoleString() << std::endl;
        
        // 显示所有权限
        std::cout << "\nUser Permissions:" << std::endl;
        auto permissions = auth_service.getCurrentUserPermissions();
        for (const auto& perm : permissions) {
            std::cout << "  - " << auth_service.permissionToString(perm) << std::endl;
        }
        
        // 测试特定权限
        std::cout << "\nSpecific Permission Tests:" << std::endl;
        std::cout << "Can CREATE_USER: " << (auth_service.hasPermission(Permission::CREATE_USER) ? "Yes" : "No") << std::endl;
        std::cout << "Can CREATE_TABLE: " << (auth_service.hasPermission(Permission::CREATE_TABLE) ? "Yes" : "No") << std::endl;
        std::cout << "Can GRANT: " << (auth_service.hasPermission(Permission::GRANT) ? "Yes" : "No") << std::endl;
        std::cout << "Can SELECT: " << (auth_service.hasPermission(Permission::SELECT) ? "Yes" : "No") << std::endl;
        
        auth_service.logout();
    } else {
        std::cout << "Login failed!" << std::endl;
    }
}

int main() {
    try {
        test_updated_permissions();
        test_auth_service_with_updated_permissions();
        
        std::cout << "\n=== All Tests Completed Successfully ===" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
