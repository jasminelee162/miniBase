#include "user_storage_manager.h"
#include "../storage/storage_engine.h"
#include "../catalog/catalog.h"
#include "../storage/page/page_utils.h"
#include <iostream>
#include <sstream>
#include <functional>

namespace minidb {

// 静态常量定义
const std::string UserStorageManager::USER_TABLE_NAME = "__users__";
const std::string UserStorageManager::USERNAME_COL = "username";
const std::string UserStorageManager::PASSWORD_COL = "password_hash";
const std::string UserStorageManager::ROLE_COL = "role";
const std::string UserStorageManager::CREATED_AT_COL = "created_at";
const std::string UserStorageManager::LAST_LOGIN_COL = "last_login";

UserStorageManager::UserStorageManager(StorageEngine* storage_engine, Catalog* catalog)
    : storage_engine_(storage_engine), catalog_(catalog), initialized_(false) {
}

bool UserStorageManager::initialize() {
    if (!storage_engine_ || !catalog_) {
        std::cerr << "[UserStorageManager] StorageEngine or Catalog not provided" << std::endl;
        return false;
    }
    
    // 检查用户表是否存在，如果不存在则创建
    if (!catalog_->HasTable(USER_TABLE_NAME)) {
        std::cout << "[UserStorageManager] Creating user table..." << std::endl;
        if (!createUserTable()) {
            std::cerr << "[UserStorageManager] Failed to create user table" << std::endl;
            return false;
        }
    }
    
    // 先设置为已初始化，然后检查是否有用户数据
    initialized_ = true;
    
    // 检查是否有用户数据，如果没有则创建默认root用户
    auto users = getAllUserRecords();
    if (users.empty()) {
        std::cout << "[UserStorageManager] Creating default root user..." << std::endl;
        if (!createUser("root", "root", Role::DBA)) {
            std::cerr << "[UserStorageManager] Failed to create default root user" << std::endl;
            return false;
        }
    }
    std::cout << "[UserStorageManager] Initialized successfully" << std::endl;
    return true;
}

bool UserStorageManager::createUserTable() {
    // 定义用户表的列结构
    std::vector<Column> columns = {
        {USERNAME_COL, "VARCHAR", 64},      // 用户名
        {PASSWORD_COL, "VARCHAR", 128},     // 密码哈希
        {ROLE_COL, "INT", -1},              // 角色（枚举值）
        {CREATED_AT_COL, "BIGINT", -1},     // 创建时间
        {LAST_LOGIN_COL, "BIGINT", -1}      // 最后登录时间
    };
    
    try {
        catalog_->CreateTable(USER_TABLE_NAME, columns);
        std::cout << "[UserStorageManager] User table created successfully" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[UserStorageManager] Failed to create user table: " << e.what() << std::endl;
        return false;
    }
}

bool UserStorageManager::createUser(const std::string& username, const std::string& password, Role role) {
    if (!initialized_) {
        std::cerr << "[UserStorageManager] Not initialized" << std::endl;
        return false;
    }
    
    if (userExists(username)) {
        std::cerr << "[UserStorageManager] User already exists: " << username << std::endl;
        return false;
    }
    
    UserRecord user(username, hashPassword(password), role);
    return insertUserRecord(user);
}

bool UserStorageManager::userExists(const std::string& username) {
    if (!initialized_) return false;
    
    try {
        UserRecord user = getUserRecord(username);
        return !user.username.empty();
    } catch (...) {
        return false;
    }
}

bool UserStorageManager::authenticate(const std::string& username, const std::string& password) {
    if (!initialized_) return false;
    
    try {
        UserRecord user = getUserRecord(username);
        if (user.username.empty()) {
            return false; // 用户不存在
        }
        
        std::string hashed_password = hashPassword(password);
        if (hashed_password == user.password_hash) {
            // 更新最后登录时间
            user.last_login = time(nullptr);
            updateUserRecord(user);
            return true;
        }
        
        return false;
    } catch (...) {
        return false;
    }
}

bool UserStorageManager::deleteUser(const std::string& username) {
    if (!initialized_) return false;
    return deleteUserRecord(username);
}

bool UserStorageManager::updateUserPassword(const std::string& username, const std::string& new_password) {
    if (!initialized_) return false;
    
    try {
        UserRecord user = getUserRecord(username);
        if (user.username.empty()) {
            return false;
        }
        
        user.password_hash = hashPassword(new_password);
        return updateUserRecord(user);
    } catch (...) {
        return false;
    }
}

bool UserStorageManager::updateUserRole(const std::string& username, Role new_role) {
    if (!initialized_) return false;
    
    try {
        UserRecord user = getUserRecord(username);
        if (user.username.empty()) {
            return false;
        }
        
        user.role = new_role;
        return updateUserRecord(user);
    } catch (...) {
        return false;
    }
}

std::vector<std::string> UserStorageManager::listUsers() {
    std::vector<std::string> usernames;
    if (!initialized_) return usernames;
    
    try {
        auto users = getAllUserRecords();
        for (const auto& user : users) {
            usernames.push_back(user.username);
        }
    } catch (...) {
        // 忽略错误，返回空列表
    }
    
    return usernames;
}

UserRecord UserStorageManager::getUserInfo(const std::string& username) {
    if (!initialized_) return UserRecord();
    
    try {
        return getUserRecord(username);
    } catch (...) {
        return UserRecord();
    }
}

Role UserStorageManager::getUserRole(const std::string& username) {
    if (!initialized_) return Role::ANALYST;
    
    try {
        UserRecord user = getUserRecord(username);
        return user.role;
    } catch (...) {
        return Role::ANALYST;
    }
}

std::string UserStorageManager::hashPassword(const std::string& password) {
    // 简单的哈希实现，实际项目中应使用更安全的哈希算法
    std::hash<std::string> hasher;
    return std::to_string(hasher(password));
}

bool UserStorageManager::insertUserRecord(const UserRecord& user) {
    try {
        // 获取用户表的第一个数据页
        TableSchema table_schema = catalog_->GetTable(USER_TABLE_NAME);
        Page* page = storage_engine_->GetDataPage(table_schema.first_page_id);
        if (!page) {
            std::cerr << "[UserStorageManager] Failed to get user table page" << std::endl;
            return false;
        }
        
        // 序列化用户记录
        std::string user_data = serializeUserRecord(user);
        
        // 将记录追加到页面
        bool success = storage_engine_->AppendRecordToPage(page, user_data.c_str(), static_cast<uint16_t>(user_data.length()));
        storage_engine_->PutPage(table_schema.first_page_id, success);
        
        if (success) {
            std::cout << "[UserStorageManager] User record inserted: " << user.username << std::endl;
        }
        
        return success;
    } catch (const std::exception& e) {
        std::cerr << "[UserStorageManager] Failed to insert user record: " << e.what() << std::endl;
        return false;
    }
}

bool UserStorageManager::updateUserRecord(const UserRecord& user) {
    // 简化实现：删除旧记录，插入新记录
    // 这样可以避免复杂的页面内更新逻辑，使用标准的数据库操作
    if (deleteUserRecord(user.username)) {
        return insertUserRecord(user);
    }
    return false;
}

bool UserStorageManager::deleteUserRecord(const std::string& username) {
    try {
        // 获取用户表的第一个数据页
        TableSchema table_schema = catalog_->GetTable(USER_TABLE_NAME);
        Page* page = storage_engine_->GetDataPage(table_schema.first_page_id);
        if (!page) {
            std::cerr << "[UserStorageManager] Failed to get user table page" << std::endl;
            return false;
        }
        
        // 获取页面中的所有记录
        auto records = storage_engine_->GetPageRecords(page);
        std::vector<UserRecord> remaining_users;
        bool found = false;
        
        // 过滤掉要删除的用户
        for (const auto& record : records) {
            std::string data(static_cast<const char*>(record.first), record.second);
            UserRecord user = deserializeUserRecord(data);
            
            if (user.username == username) {
                found = true;
                std::cout << "[UserStorageManager] Found user to delete: " << username << std::endl;
            } else if (!user.username.empty()) {
                remaining_users.push_back(user);
            }
        }
        
        if (!found) {
            std::cout << "[UserStorageManager] User not found: " << username << std::endl;
            storage_engine_->PutPage(table_schema.first_page_id, false);
            return false;
        }
        
        // 重新初始化页面并插入剩余的用户记录
        storage_engine_->InitializeDataPage(page);
        for (const auto& user : remaining_users) {
            std::string user_data = serializeUserRecord(user);
            bool success = storage_engine_->AppendRecordToPage(page, user_data.c_str(), static_cast<uint16_t>(user_data.length()));
            if (!success) {
                std::cerr << "[UserStorageManager] Failed to re-insert user: " << user.username << std::endl;
                storage_engine_->PutPage(table_schema.first_page_id, false);
                return false;
            }
        }
        
        storage_engine_->PutPage(table_schema.first_page_id, true);
        std::cout << "[UserStorageManager] User record deleted successfully: " << username << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "[UserStorageManager] Failed to delete user record: " << e.what() << std::endl;
        return false;
    }
}

std::vector<UserRecord> UserStorageManager::getAllUserRecords() {
    std::vector<UserRecord> users;
    
    try {
        // 获取用户表的第一个数据页
        TableSchema table_schema = catalog_->GetTable(USER_TABLE_NAME);
        Page* page = storage_engine_->GetDataPage(table_schema.first_page_id);
        if (!page) {
            return users;
        }
        
        // 获取页面中的所有记录
        auto records = storage_engine_->GetPageRecords(page);
        for (const auto& record : records) {
            std::string data(static_cast<const char*>(record.first), record.second);
            UserRecord user = deserializeUserRecord(data);
            if (!user.username.empty()) {
                users.push_back(user);
            }
        }
        
        storage_engine_->PutPage(table_schema.first_page_id, false);
    } catch (const std::exception& e) {
        std::cerr << "[UserStorageManager] Failed to get user records: " << e.what() << std::endl;
    }
    
    return users;
}

UserRecord UserStorageManager::getUserRecord(const std::string& username) {
    auto users = getAllUserRecords();
    for (const auto& user : users) {
        if (user.username == username) {
            return user;
        }
    }
    return UserRecord(); // 返回空记录表示未找到
}

std::string UserStorageManager::serializeUserRecord(const UserRecord& user) const {
    std::ostringstream oss;
    oss << user.username << "|" 
        << user.password_hash << "|" 
        << static_cast<int>(user.role) << "|" 
        << user.created_at << "|" 
        << user.last_login;
    return oss.str();
}

UserRecord UserStorageManager::deserializeUserRecord(const std::string& data) const {
    UserRecord user;
    std::istringstream iss(data);
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

} // namespace minidb
