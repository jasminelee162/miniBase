#include "auth_service.h"
#include "../storage/storage_engine.h"
#include "../catalog/catalog.h"
#include <iostream>

namespace minidb {

AuthService::AuthService() : current_user_(""), is_logged_in_(false), use_storage_engine_(false) {
    // 加载用户数据
    loadFromFile("users.dat");
}

AuthService::AuthService(StorageEngine* storage_engine, Catalog* catalog) 
    : current_user_(""), is_logged_in_(false), use_storage_engine_(true) {
    // 使用存储引擎初始化用户管理
    user_storage_manager_ = std::make_unique<UserStorageManager>(storage_engine, catalog);
    if (!user_storage_manager_->initialize()) {
        std::cerr << "[AuthService] Failed to initialize user storage manager" << std::endl;
        use_storage_engine_ = false;
    }
}

bool AuthService::login(const std::string& username, const std::string& password) {
    bool success = false;
    
    if (use_storage_engine_ && user_storage_manager_) {
        success = user_storage_manager_->authenticate(username, password);
    } else {
        success = user_manager_.authenticate(username, password);
    }
    
    if (success) {
        current_user_ = username;
        is_logged_in_ = true;
        return true;
    }
    return false;
}

void AuthService::logout() {
    current_user_.clear();
    is_logged_in_ = false;
}

bool AuthService::isLoggedIn() const {
    return is_logged_in_;
}

std::string AuthService::getCurrentUser() const {
    return current_user_;
}

bool AuthService::createUser(const std::string& username, const std::string& password, Role role) {
    // 检查当前用户是否有权限创建用户
    if (!isDBA()) {
        return false;
    }
    
    if (use_storage_engine_ && user_storage_manager_) {
        return user_storage_manager_->createUser(username, password, role);
    } else {
        return user_manager_.createUser(username, password, role);
    }
}

bool AuthService::userExists(const std::string& username) const {
    if (use_storage_engine_ && user_storage_manager_) {
        return user_storage_manager_->userExists(username);
    } else {
        return user_manager_.userExists(username);
    }
}

bool AuthService::deleteUser(const std::string& username) {
    // 检查当前用户是否有权限删除用户
    if (!isDBA()) {
        return false;
    }
    
    if (use_storage_engine_ && user_storage_manager_) {
        return user_storage_manager_->deleteUser(username);
    } else {
        return user_manager_.deleteUser(username);
    }
}

std::vector<std::string> AuthService::listUsers() const {
    // 检查当前用户是否有权限查看用户列表
    if (!isDBA()) {
        return {};
    }
    
    if (use_storage_engine_ && user_storage_manager_) {
        return user_storage_manager_->listUsers();
    } else {
        return user_manager_.listUsers();
    }
}

std::vector<UserInfo> AuthService::getAllUsers() const {
    // 检查当前用户是否有权限查看用户列表
    if (!isDBA()) {
        return {};
    }
    
    return user_manager_.getAllUsers();
}

bool AuthService::hasPermission(Permission permission) const {
    if (!is_logged_in_) return false;
    
    if (use_storage_engine_ && user_storage_manager_) {
        // 从存储引擎获取用户角色
        Role user_role = user_storage_manager_->getUserRole(current_user_);
        RoleManager role_manager;
        return role_manager.hasPermission(user_role, permission);
    } else {
        return user_manager_.hasPermission(current_user_, permission);
    }
}

Role AuthService::getCurrentUserRole() const {
    if (!is_logged_in_) return Role::ANALYST;
    
    if (use_storage_engine_ && user_storage_manager_) {
        return user_storage_manager_->getUserRole(current_user_);
    } else {
        return user_manager_.getUserRole(current_user_);
    }
}

bool AuthService::isDBA() const {
    return getCurrentUserRole() == Role::DBA;
}

std::string AuthService::getCurrentUserRoleString() const {
    RoleManager role_manager;
    return role_manager.roleToString(getCurrentUserRole());
}

std::vector<Permission> AuthService::getCurrentUserPermissions() const {
    if (!is_logged_in_) return {};
    
    RoleManager role_manager;
    return role_manager.getRolePermissions(getCurrentUserRole());
}

bool AuthService::saveToFile(const std::string& filename) const {
    return user_manager_.saveToFile(filename);
}

bool AuthService::loadFromFile(const std::string& filename) {
    return user_manager_.loadFromFile(filename);
}

void AuthService::setCurrentUser(const std::string& username) {
    current_user_ = username;
    is_logged_in_ = !username.empty();
}

std::string AuthService::permissionToString(Permission permission) const {
    RoleManager role_manager;
    return role_manager.permissionToString(permission);
}

} // namespace minidb
