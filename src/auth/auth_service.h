#pragma once
#include <string>
#include <vector>
#include "user_manager.h"
#include "user_storage_manager.h"

// 前向声明
namespace minidb {
    class StorageEngine;
    class Catalog;
}

namespace minidb {

class AuthService {
private:
    UserManager user_manager_;
    std::unique_ptr<UserStorageManager> user_storage_manager_;
    std::string current_user_;
    bool is_logged_in_;
    bool use_storage_engine_;
    
public:
    AuthService();
    AuthService(StorageEngine* storage_engine, Catalog* catalog);
    
    // 认证相关
    bool login(const std::string& username, const std::string& password);
    void logout();
    bool isLoggedIn() const;
    std::string getCurrentUser() const;
    
    // 用户管理（需要DBA权限）
    bool createUser(const std::string& username, const std::string& password, Role role);
    bool userExists(const std::string& username) const;
    bool deleteUser(const std::string& username);
    std::vector<std::string> listUsers() const;
    std::vector<UserInfo> getAllUsers() const;
    
    // 权限检查
    bool hasPermission(Permission permission) const;
    Role getCurrentUserRole() const;
    bool isDBA() const;
    
    // 角色管理
    std::string getCurrentUserRoleString() const;
    std::vector<Permission> getCurrentUserPermissions() const;
    std::string permissionToString(Permission permission) const;
    
    // 持久化
    bool saveToFile(const std::string& filename) const;
    bool loadFromFile(const std::string& filename);
    
    // 工具方法
    void setCurrentUser(const std::string& username);
};

} // namespace minidb
