#include "auth_service.h"
#include "../storage/storage_engine.h"
#include "../catalog/catalog.h"
#include <iostream>

namespace minidb
{

    AuthService::AuthService(StorageEngine *storage_engine, Catalog *catalog)
        : catalog_(catalog), current_user_(""), is_logged_in_(false), storage_engine_(storage_engine)
    {
        // 使用存储引擎初始化用户管理
        user_storage_manager_ = std::make_unique<UserStorageManager>(storage_engine, catalog);
        if (!user_storage_manager_->initialize())
        {
            std::cerr << "[AuthService] Failed to initialize user storage manager" << std::endl;
            throw std::runtime_error("Failed to initialize user storage manager");
        }
    }

    bool AuthService::login(const std::string &username, const std::string &password)
    {
        // 安全检查：如果已经登录，不允许重复登录
        if (is_logged_in_)
        {
            std::cerr << "[AuthService] Already logged in as: " << current_user_
                      << ". Please logout first before logging in as another user." << std::endl;
            return false;
        }

        // 使用存储引擎进行认证
        if (user_storage_manager_->authenticate(username, password))
        {
            current_user_ = username;
            is_logged_in_ = true;
            return true;
        }
        return false;
    }

    void AuthService::logout()
    {
        current_user_.clear();
        is_logged_in_ = false;
    }

    bool AuthService::isLoggedIn() const
    {
        return is_logged_in_;
    }

    std::string AuthService::getCurrentUser() const
    {
        return current_user_;
    }

    bool AuthService::createUser(const std::string &username, const std::string &password, Role role)
    {
        // 安全检查：必须先登录
        if (!is_logged_in_)
        {
            std::cerr << "[AuthService] Must be logged in to create users." << std::endl;
            return false;
        }

        // 检查当前用户是否有权限创建用户
        if (!isDBA())
        {
            std::cerr << "[AuthService] Only DBA users can create other users." << std::endl;
            return false;
        }

        bool ok = user_storage_manager_->createUser(username, password, role);
        if (ok && storage_engine_) {
            // 立即刷盘，确保意外退出也能保存
            storage_engine_->Checkpoint();
        }
        return ok;
    }

    bool AuthService::userExists(const std::string &username) const
    {
        return user_storage_manager_->userExists(username);
    }

    bool AuthService::deleteUser(const std::string &username)
    {
        // 安全检查：必须先登录
        if (!is_logged_in_)
        {
            std::cerr << "[AuthService] Must be logged in to delete users." << std::endl;
            return false;
        }

        // 检查当前用户是否有权限删除用户
        if (!isDBA())
        {
            std::cerr << "[AuthService] Only DBA users can delete other users." << std::endl;
            return false;
        }

        return user_storage_manager_->deleteUser(username);
    }

    std::vector<std::string> AuthService::listUsers() const
    {
        // 安全检查：必须先登录
        if (!is_logged_in_)
        {
            std::cerr << "[AuthService] Must be logged in to list users." << std::endl;
            return {};
        }

        // 检查当前用户是否有权限查看用户列表
        if (!isDBA())
        {
            std::cerr << "[AuthService] Only DBA users can list other users." << std::endl;
            return {};
        }

        return user_storage_manager_->listUsers();
    }

    std::vector<UserInfo> AuthService::getAllUsers() const
    {
        // 安全检查：必须先登录
        if (!is_logged_in_)
        {
            std::cerr << "[AuthService] Must be logged in to get user information." << std::endl;
            return {};
        }

        // 检查当前用户是否有权限查看用户列表
        if (!isDBA())
        {
            std::cerr << "[AuthService] Only DBA users can get user information." << std::endl;
            return {};
        }

        auto user_records = user_storage_manager_->getAllUsers();
        std::vector<UserInfo> user_infos;
        for (const auto &record : user_records)
        {
            UserInfo info;
            info.username = record.username;
            info.password_hash = record.password_hash;
            info.role = record.role;
            info.created_at = record.created_at;
            info.last_login = record.last_login;
            user_infos.push_back(info);
        }
        return user_infos;
    }

    bool AuthService::hasPermission(Permission permission) const
    {
        if (!is_logged_in_)
            return false;

        // 从存储引擎获取用户角色
        Role user_role = user_storage_manager_->getUserRole(current_user_);
        RoleManager role_manager;
        return role_manager.hasPermission(user_role, permission);
    }

    Role AuthService::getCurrentUserRole() const
    {
        if (!is_logged_in_)
            return Role::ANALYST;

        return user_storage_manager_->getUserRole(current_user_);
    }

    bool AuthService::isDBA() const
    {
        return getCurrentUserRole() == Role::DBA;
    }

    std::string AuthService::getCurrentUserRoleString() const
    {
        RoleManager role_manager;
        return role_manager.roleToString(getCurrentUserRole());
    }

    std::vector<Permission> AuthService::getCurrentUserPermissions() const
    {
        if (!is_logged_in_)
            return {};

        RoleManager role_manager;
        return role_manager.getRolePermissions(getCurrentUserRole());
    }

    // 文件持久化方法已移除，数据通过存储引擎自动持久化

    void AuthService::setCurrentUser(const std::string &username)
    {
        current_user_ = username;
        is_logged_in_ = !username.empty();
    }

    bool AuthService::checkTablePermission(const std::string &table_name, Permission permission) const
    {
        if (!is_logged_in_ || !catalog_)
            return false;

        // 获取当前用户和角色
        std::string current_user = getCurrentUser();
        Role user_role = getCurrentUserRole();

        // 系统表保护：__users__ 仅 DBA 可见/可操作
        if (table_name == "__users__")
        {
            return user_role == Role::DBA && hasPermission(permission);
        }

        // DBA可以操作所有表
        if (user_role == Role::DBA)
        {
            return hasPermission(permission);
        }

        // DEVELOPER只能操作自己创建的表
        if (user_role == Role::DEVELOPER)
        {
            if (catalog_->IsTableOwner(table_name, current_user))
            {
                return hasPermission(permission);
            }
            return false; // 不能操作其他用户的表
        }

        // ANALYST只能查看所有表
        if (user_role == Role::ANALYST)
        {
            return permission == Permission::SELECT;
        }

        return false;
    }

    std::string AuthService::permissionToString(Permission permission) const
    {
        RoleManager role_manager;
        return role_manager.permissionToString(permission);
    }

} // namespace minidb