#include "auth_service.h"
#include <iostream>

using namespace minidb;

void demonstrate_auth_usage() {
    std::cout << "=== MiniBase Auth Module Usage Example ===" << std::endl;
    
    // 1. 创建认证服务
    AuthService auth_service;
    
    // 2. 用户登录
    std::cout << "\n1. User Login:" << std::endl;
    if (auth_service.login("admin", "admin123")) {
        std::cout << "Login successful!" << std::endl;
        std::cout << "Current user: " << auth_service.getCurrentUser() << std::endl;
        std::cout << "Role: " << auth_service.getCurrentUserRoleString() << std::endl;
    } else {
        std::cout << "Login failed!" << std::endl;
        return;
    }
    
    // 3. 显示用户权限
    std::cout << "\n2. User Permissions:" << std::endl;
    auto permissions = auth_service.getCurrentUserPermissions();
    for (const auto& perm : permissions) {
        RoleManager role_manager;
        std::cout << "  - " << role_manager.permissionToString(perm) << std::endl;
    }
    
    // 4. 创建新用户（DBA权限）
    std::cout << "\n3. Creating New Users:" << std::endl;
    if (auth_service.createUser("developer1", "dev123", Role::DEVELOPER)) {
        std::cout << "Developer user created successfully!" << std::endl;
    }
    
    if (auth_service.createUser("analyst1", "analyst123", Role::ANALYST)) {
        std::cout << "Analyst user created successfully!" << std::endl;
    }
    
    // 5. 列出所有用户
    std::cout << "\n4. All Users:" << std::endl;
    auto users = auth_service.getAllUsers();
    RoleManager role_manager;
    for (const auto& user : users) {
        std::cout << "  - " << user.username << " (" << role_manager.roleToString(user.role) << ")" << std::endl;
    }
    
    // 6. 测试权限检查
    std::cout << "\n5. Permission Testing:" << std::endl;
    std::cout << "Can CREATE_TABLE: " << (auth_service.hasPermission(Permission::CREATE_TABLE) ? "Yes" : "No") << std::endl;
    std::cout << "Can CREATE_USER: " << (auth_service.hasPermission(Permission::CREATE_USER) ? "Yes" : "No") << std::endl;
    std::cout << "Can SELECT: " << (auth_service.hasPermission(Permission::SELECT) ? "Yes" : "No") << std::endl;
    
    // 7. 切换到其他用户
    std::cout << "\n6. Switching to Developer User:" << std::endl;
    auth_service.logout();
    if (auth_service.login("developer1", "dev123")) {
        std::cout << "Switched to developer1" << std::endl;
        std::cout << "Role: " << auth_service.getCurrentUserRoleString() << std::endl;
        std::cout << "Can CREATE_TABLE: " << (auth_service.hasPermission(Permission::CREATE_TABLE) ? "Yes" : "No") << std::endl;
        std::cout << "Can CREATE_USER: " << (auth_service.hasPermission(Permission::CREATE_USER) ? "Yes" : "No") << std::endl;
    }
    
    // 8. 切换到分析师用户
    std::cout << "\n7. Switching to Analyst User:" << std::endl;
    auth_service.logout();
    if (auth_service.login("analyst1", "analyst123")) {
        std::cout << "Switched to analyst1" << std::endl;
        std::cout << "Role: " << auth_service.getCurrentUserRoleString() << std::endl;
        std::cout << "Can SELECT: " << (auth_service.hasPermission(Permission::SELECT) ? "Yes" : "No") << std::endl;
        std::cout << "Can INSERT: " << (auth_service.hasPermission(Permission::INSERT) ? "Yes" : "No") << std::endl;
    }
    
    // 9. 保存用户数据
    std::cout << "\n8. Saving User Data:" << std::endl;
    if (auth_service.saveToFile("users.dat")) {
        std::cout << "User data saved successfully!" << std::endl;
    } else {
        std::cout << "Failed to save user data!" << std::endl;
    }
    
    std::cout << "\n=== Demo Complete ===" << std::endl;
}

int main() {
    demonstrate_auth_usage();
    return 0;
}
