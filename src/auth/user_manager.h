#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <ctime>
#include <fstream>
#include <sstream>
#include "role_manager.h"

// 前向声明
namespace minidb {
    class StorageEngine;
    class Catalog;
}

namespace minidb {

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

class UserManager {
private:
    std::unordered_map<std::string, UserInfo> users_;
    RoleManager role_manager_;
    
    // 存储引擎集成
    StorageEngine* storage_engine_;
    Catalog* catalog_;
    bool use_storage_engine_;
    
    std::string hashPassword(const std::string& password);
    std::string generateSalt();
    std::string serializeUser(const UserInfo& user) const;
    UserInfo deserializeUser(const std::string& line) const;
    
    // 存储引擎相关方法
    bool initializeUserTable();
    bool loadUsersFromStorage();
    bool saveUsersToStorage();
    bool insertUserToStorage(const UserInfo& user);
    bool deleteUserFromStorage(const std::string& username);
    
public:
    UserManager();
    UserManager(StorageEngine* storage_engine, Catalog* catalog);
    
    // 用户管理
    bool createUser(const std::string& username, const std::string& password, Role role);
    bool userExists(const std::string& username) const;
    bool authenticate(const std::string& username, const std::string& password);
    bool deleteUser(const std::string& username);
    
    // 权限检查
    bool hasPermission(const std::string& username, Permission permission) const;
    Role getUserRole(const std::string& username) const;
    bool isDBA(const std::string& username) const;
    
    // 用户列表（仅DBA可用）
    std::vector<std::string> listUsers() const;
    std::vector<UserInfo> getAllUsers() const;
    
    // 持久化
    bool saveToFile(const std::string& filename) const;
    bool loadFromFile(const std::string& filename);
    
    // 工具方法
    std::string getCurrentUser() const { return current_user_; }
    void setCurrentUser(const std::string& username) { current_user_ = username; }
    
private:
    std::string current_user_;
};

} // namespace minidb
