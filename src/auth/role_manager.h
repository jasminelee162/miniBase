#pragma once
#include <unordered_map>
#include <set>
#include <string>
#include <vector>

namespace minidb {

enum class Role {
    DBA,        // 数据库管理员
    DEVELOPER,  // 开发人员
    ANALYST     // 数据分析人员
};

enum class Permission {
    // 数据库管理权限
    CREATE_DATABASE, DROP_DATABASE,
    BACKUP, RESTORE,
    
    // 表结构权限
    CREATE_TABLE, DROP_TABLE, ALTER_TABLE,
    CREATE_INDEX, DROP_INDEX,
    
    // 数据操作权限
    SELECT, INSERT, UPDATE, DELETE,
    
    // 用户管理权限
    CREATE_USER, DROP_USER, GRANT, REVOKE,
    
    // 系统管理权限
    SHOW_PROCESSES, KILL_PROCESS,
    SHOW_VARIABLES, SET_VARIABLES
};

class RoleManager {
private:
    std::unordered_map<Role, std::set<Permission>> role_permissions_;
    
    void initializeRolePermissions();
    
public:
    RoleManager();
    
    // 权限检查
    bool hasPermission(Role role, Permission permission) const;
    
    // 角色和权限转换
    std::string roleToString(Role role) const;
    std::string permissionToString(Permission permission) const;
    Role stringToRole(const std::string& role_str) const;
    
    // 获取角色权限列表
    std::vector<Permission> getRolePermissions(Role role) const;
    
    // 获取所有角色
    std::vector<Role> getAllRoles() const;
};

} // namespace minidb
