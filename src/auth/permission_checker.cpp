#include "permission_checker.h"
#include <algorithm>
#include <cctype>

namespace minidb {

PermissionChecker::PermissionChecker(AuthService* auth_service) 
    : auth_service_(auth_service) {
}

bool PermissionChecker::checkPermission(Permission permission) const {
    if (!auth_service_) return false;
    return auth_service_->hasPermission(permission);
}

bool PermissionChecker::checkTablePermission(const std::string& table_name, Permission permission) const {
    if (!auth_service_) return false;
    
    // 这里可以添加表级别的权限检查
    // 目前只检查基本权限
    (void)table_name; // 避免未使用参数警告
    return auth_service_->hasPermission(permission);
}

bool PermissionChecker::checkSQLPermission(const std::string& sql) const {
    if (!auth_service_) return false;
    
    // 将SQL转换为大写进行简单分析
    std::string upper_sql = sql;
    std::transform(upper_sql.begin(), upper_sql.end(), upper_sql.begin(), ::toupper);
    
    // 去除前后空格
    upper_sql.erase(0, upper_sql.find_first_not_of(" \t\r\n"));
    upper_sql.erase(upper_sql.find_last_not_of(" \t\r\n") + 1);
    
    // 检查SQL语句类型并验证权限
    if (upper_sql.find("SELECT") == 0) {
        return auth_service_->hasPermission(Permission::SELECT);
    } else if (upper_sql.find("INSERT") == 0) {
        return auth_service_->hasPermission(Permission::INSERT);
    } else if (upper_sql.find("UPDATE") == 0) {
        return auth_service_->hasPermission(Permission::UPDATE);
    } else if (upper_sql.find("DELETE") == 0) {
        return auth_service_->hasPermission(Permission::DELETE);
    } else if (upper_sql.find("CREATE TABLE") == 0) {
        return auth_service_->hasPermission(Permission::CREATE_TABLE);
    } else if (upper_sql.find("DROP TABLE") == 0) {
        return auth_service_->hasPermission(Permission::DROP_TABLE);
    } else if (upper_sql.find("ALTER TABLE") == 0) {
        return auth_service_->hasPermission(Permission::ALTER_TABLE);
    } else if (upper_sql.find("CREATE INDEX") == 0) {
        return auth_service_->hasPermission(Permission::CREATE_INDEX);
    } else if (upper_sql.find("DROP INDEX") == 0) {
        return auth_service_->hasPermission(Permission::DROP_INDEX);
    }
    
    // 默认允许（可能是其他类型的SQL）
    return true;
}

} // namespace minidb