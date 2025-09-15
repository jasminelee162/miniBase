#pragma once
#include <string>
#include <vector>
#include <ctime>
#include "user_storage_manager.h"
#include "role_manager.h"

// 前向声明
namespace minidb {
    class StorageEngine;
    class Catalog;
}

namespace minidb {

// 用户信息结构体（与UserRecord兼容）
struct UserInfo {
    std::string username;
    std::string password_hash;
    Role role;
    time_t created_at;
    time_t last_login;
    
    UserInfo() : role(Role::ANALYST), created_at(0), last_login(0) {}
    
    UserInfo(const std::string& uname, const std::string& phash, Role r)
        : username(uname), password_hash(phash), role(r), created_at(time(nullptr)), last_login(0) {}
};

class AuthService {
private:
    std::unique_ptr<UserStorageManager> user_storage_manager_;
    Catalog* catalog_;  // 用于表权限检查
    std::string current_user_;
    bool is_logged_in_;
    
public:
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
    
    // 持久化（通过存储引擎自动处理）
    
    // 工具方法
    void setCurrentUser(const std::string& username);
    
    // 表权限检查（需要Catalog访问）
    bool checkTablePermission(const std::string& table_name, Permission permission) const;
};

} // namespace minidb
