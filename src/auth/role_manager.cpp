#include "role_manager.h"
#include <algorithm>

namespace minidb {

RoleManager::RoleManager() {
    initializeRolePermissions();
}

void RoleManager::initializeRolePermissions() {
    // DBA：所有权限
    role_permissions_[Role::DBA] = {
        Permission::CREATE_DATABASE, Permission::DROP_DATABASE,
        Permission::BACKUP, Permission::RESTORE,
        Permission::CREATE_TABLE, Permission::DROP_TABLE, Permission::ALTER_TABLE,
        Permission::CREATE_INDEX, Permission::DROP_INDEX,
        Permission::SELECT, Permission::INSERT, Permission::UPDATE, Permission::DELETE,
        Permission::CREATE_USER, Permission::DROP_USER, Permission::GRANT, Permission::REVOKE,
        Permission::SHOW_PROCESSES, Permission::KILL_PROCESS,
        Permission::SHOW_VARIABLES, Permission::SET_VARIABLES
    };
    
    // DEVELOPER：数据操作和表结构权限
    role_permissions_[Role::DEVELOPER] = {
        Permission::CREATE_TABLE, Permission::DROP_TABLE, Permission::ALTER_TABLE,
        Permission::CREATE_INDEX, Permission::DROP_INDEX,
        Permission::SELECT, Permission::INSERT, Permission::UPDATE, Permission::DELETE
    };
    
    // ANALYST：只读权限
    role_permissions_[Role::ANALYST] = {
        Permission::SELECT
    };
}

bool RoleManager::hasPermission(Role role, Permission permission) const {
    auto it = role_permissions_.find(role);
    if (it == role_permissions_.end()) return false;
    return it->second.count(permission) > 0;
}

std::string RoleManager::roleToString(Role role) const {
    switch (role) {
        case Role::DBA: return "DBA";
        case Role::DEVELOPER: return "DEVELOPER";
        case Role::ANALYST: return "ANALYST";
        default: return "UNKNOWN";
    }
}

std::string RoleManager::permissionToString(Permission permission) const {
    switch (permission) {
        case Permission::CREATE_DATABASE: return "CREATE_DATABASE";
        case Permission::DROP_DATABASE: return "DROP_DATABASE";
        case Permission::BACKUP: return "BACKUP";
        case Permission::RESTORE: return "RESTORE";
        case Permission::CREATE_TABLE: return "CREATE_TABLE";
        case Permission::DROP_TABLE: return "DROP_TABLE";
        case Permission::ALTER_TABLE: return "ALTER_TABLE";
        case Permission::CREATE_INDEX: return "CREATE_INDEX";
        case Permission::DROP_INDEX: return "DROP_INDEX";
        case Permission::SELECT: return "SELECT";
        case Permission::INSERT: return "INSERT";
        case Permission::UPDATE: return "UPDATE";
        case Permission::DELETE: return "DELETE";
        case Permission::CREATE_USER: return "CREATE_USER";
        case Permission::DROP_USER: return "DROP_USER";
        case Permission::GRANT: return "GRANT";
        case Permission::REVOKE: return "REVOKE";
        case Permission::SHOW_PROCESSES: return "SHOW_PROCESSES";
        case Permission::KILL_PROCESS: return "KILL_PROCESS";
        case Permission::SHOW_VARIABLES: return "SHOW_VARIABLES";
        case Permission::SET_VARIABLES: return "SET_VARIABLES";
        default: return "UNKNOWN";
    }
}

Role RoleManager::stringToRole(const std::string& role_str) const {
    if (role_str == "DBA") return Role::DBA;
    if (role_str == "DEVELOPER") return Role::DEVELOPER;
    if (role_str == "ANALYST") return Role::ANALYST;
    return Role::ANALYST; // 默认最低权限
}

std::vector<Permission> RoleManager::getRolePermissions(Role role) const {
    auto it = role_permissions_.find(role);
    if (it == role_permissions_.end()) return {};
    
    std::vector<Permission> permissions;
    for (const auto& perm : it->second) {
        permissions.push_back(perm);
    }
    return permissions;
}

std::vector<Role> RoleManager::getAllRoles() const {
    return {Role::DBA, Role::DEVELOPER, Role::ANALYST};
}

} // namespace minidb
