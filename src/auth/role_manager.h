#pragma once
#include <map>
#include <set>
#include <string>
#include <vector>

namespace minidb
{

    enum class Role
    {
        DBA,       // 数据库管理员
        DEVELOPER, // 开发人员
        ANALYST    // 数据分析人员
    };

    // 为Role枚举添加比较操作符，使std::unordered_map可以正常工作
    inline bool operator==(Role a, Role b)
    {
        return static_cast<int>(a) == static_cast<int>(b);
    }

    enum class Permission
    {
        // 表结构权限
        CREATE_TABLE,
        DROP_TABLE,
        ALTER_TABLE,
        CREATE_INDEX,
        DROP_INDEX,

        // 数据操作权限
        SELECT,
        INSERT,
        UPDATE,
        DELETE,

        // 存储过程权限
        CREATE_PROCEDURE, // 新增：创建存储过程
        DROP_PROCEDURE,   // 新增：删除存储过程
        CALL_PROCEDURE,   // 新增：调用存储过程

        // 用户管理权限
        CREATE_USER,
        DROP_USER,
        GRANT,
        REVOKE,

        // 系统管理权限
        SHOW_PROCESSES,
        KILL_PROCESS,
        SHOW_VARIABLES,
        SET_VARIABLES
    };

    // 为Permission枚举添加比较操作符，使std::set可以正常工作
    inline bool operator<(Permission a, Permission b)
    {
        return static_cast<int>(a) < static_cast<int>(b);
    }

    class RoleManager
    {
    private:
        std::map<Role, std::set<Permission>> role_permissions_;

        void initializeRolePermissions();

    public:
        RoleManager();

        // 权限检查
        bool hasPermission(Role role, Permission permission) const;

        // 角色和权限转换
        std::string roleToString(Role role) const;
        std::string permissionToString(Permission permission) const;
        Role stringToRole(const std::string &role_str) const;

        // 获取角色权限列表
        std::vector<Permission> getRolePermissions(Role role) const;

        // 获取所有角色
        std::vector<Role> getAllRoles() const;
    };

} // namespace minidb
