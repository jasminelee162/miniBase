#pragma once
#include "auth_service.h"
#include <string>

namespace minidb {

class PermissionChecker {
private:
    AuthService* auth_service_;
    
public:
    PermissionChecker(AuthService* auth_service);
    
    // 检查特定操作权限
    bool checkPermission(Permission permission) const;
    
    // 检查表操作权限
    bool checkTablePermission(const std::string& table_name, Permission permission) const;
    
    // 检查SQL语句权限
    bool checkSQLPermission(const std::string& sql) const;
};

} // namespace minidb