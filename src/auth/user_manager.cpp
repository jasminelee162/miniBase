#include "user_manager.h"
#include "../storage/storage_engine.h"
#include "../catalog/catalog.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <random>
#include <chrono>

namespace minidb {

UserManager::UserManager() : current_user_("") {
    // 创建默认DBA用户
    if (!userExists("admin")) {
        createUser("admin", "admin123", Role::DBA);
    }
}

std::string UserManager::hashPassword(const std::string& password) {
    // 简单的哈希实现，实际项目中应使用更安全的哈希算法
    std::hash<std::string> hasher;
    return std::to_string(hasher(password));
}

std::string UserManager::generateSalt() {
    // 生成简单的盐值（避免使用可能阻塞的random_device）
    static int counter = 0;
    return std::to_string(1000 + (counter++ % 9000));
}

bool UserManager::createUser(const std::string& username, const std::string& password, Role role) {
    if (userExists(username)) {
        return false; // 用户已存在
    }
    
    if (username.empty() || password.empty()) {
        return false; // 用户名或密码为空
    }
    
    // 简化处理：直接使用密码哈希（实际项目中应该使用盐值）
    std::string hashed_password = hashPassword(password);
    
    UserInfo user(username, hashed_password, role);
    users_[username] = user;
    
    return true;
}

bool UserManager::userExists(const std::string& username) const {
    return users_.find(username) != users_.end();
}

bool UserManager::authenticate(const std::string& username, const std::string& password) {
    auto it = users_.find(username);
    if (it == users_.end()) {
        return false; // 用户不存在
    }
    
    // 简化处理：直接比较密码（实际项目中应该使用盐值）
    std::string hashed_password = hashPassword(password);
    if (hashed_password == it->second.password_hash) {
        it->second.last_login = time(nullptr);
        current_user_ = username;
        return true;
    }
    
    return false;
}

bool UserManager::deleteUser(const std::string& username) {
    auto it = users_.find(username);
    if (it == users_.end()) {
        return false; // 用户不存在
    }
    
    users_.erase(it);
    return true;
}

bool UserManager::hasPermission(const std::string& username, Permission permission) const {
    auto it = users_.find(username);
    if (it == users_.end()) return false;
    
    return role_manager_.hasPermission(it->second.role, permission);
}

Role UserManager::getUserRole(const std::string& username) const {
    auto it = users_.find(username);
    if (it == users_.end()) return Role::ANALYST; // 默认最低权限
    return it->second.role;
}

bool UserManager::isDBA(const std::string& username) const {
    return getUserRole(username) == Role::DBA;
}

std::vector<std::string> UserManager::listUsers() const {
    std::vector<std::string> user_list;
    for (const auto& pair : users_) {
        user_list.push_back(pair.first);
    }
    return user_list;
}

std::vector<UserInfo> UserManager::getAllUsers() const {
    std::vector<UserInfo> user_list;
    for (const auto& pair : users_) {
        user_list.push_back(pair.second);
    }
    return user_list;
}

std::string UserManager::serializeUser(const UserInfo& user) const {
    std::ostringstream oss;
    oss << user.username << "|" 
        << user.password_hash << "|" 
        << static_cast<int>(user.role) << "|" 
        << user.created_at << "|" 
        << user.last_login;
    return oss.str();
}

UserInfo UserManager::deserializeUser(const std::string& line) const {
    UserInfo user;
    std::istringstream iss(line);
    std::string token;
    
    // 解析用户名
    if (std::getline(iss, token, '|')) {
        user.username = token;
    }
    
    // 解析密码哈希
    if (std::getline(iss, token, '|')) {
        user.password_hash = token;
    }
    
    // 解析角色
    if (std::getline(iss, token, '|')) {
        int role_int = std::stoi(token);
        user.role = static_cast<Role>(role_int);
    }
    
    // 解析创建时间
    if (std::getline(iss, token, '|')) {
        user.created_at = std::stol(token);
    }
    
    // 解析最后登录时间
    if (std::getline(iss, token, '|')) {
        user.last_login = std::stol(token);
    }
    
    return user;
}

bool UserManager::saveToFile(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    for (const auto& pair : users_) {
        file << serializeUser(pair.second) << std::endl;
    }
    
    file.close();
    return true;
}

bool UserManager::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    users_.clear();
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) {
            UserInfo user = deserializeUser(line);
            users_[user.username] = user;
        }
    }
    
    file.close();
    return true;
}

} // namespace minidb
