# 权限管理模块 API 参考

## 概述

权限管理模块提供了完整的用户认证、授权和权限管理功能，可以安全地集成到CLI或其他应用程序中。

## 核心类

### AuthService

主要的权限管理服务类，提供所有权限管理相关的接口。

#### 构造函数
```cpp
// 使用存储引擎的构造函数（推荐）
AuthService(StorageEngine* storage_engine, Catalog* catalog);

```

#### 用户认证接口

##### 登录
```cpp
bool login(const std::string& username, const std::string& password);
```
- **功能**: 用户登录
- **参数**: 
  - `username`: 用户名
  - `password`: 密码
- **返回值**: 登录成功返回true，失败返回false
- **异常**: 不会抛出异常

##### 登出
```cpp
void logout();
```
- **功能**: 用户登出
- **异常**: 不会抛出异常

##### 检查登录状态
```cpp
bool isLoggedIn() const;
```
- **功能**: 检查当前是否有用户登录
- **返回值**: 已登录返回true，未登录返回false

##### 获取当前用户
```cpp
std::string getCurrentUser() const;
```
- **功能**: 获取当前登录的用户名
- **返回值**: 当前用户名，未登录时返回空字符串

##### 获取当前用户角色
```cpp
Role getCurrentUserRole() const;
```
- **功能**: 获取当前用户的角色
- **返回值**: 用户角色枚举值（DBA, DEVELOPER, ANALYST）

##### 检查是否为DBA
```cpp
bool isDBA() const;
```
- **功能**: 检查当前用户是否为DBA
- **返回值**: 是DBA返回true，否则返回false

#### 用户管理接口

##### 创建用户
```cpp
bool createUser(const std::string& username, const std::string& password, Role role);
```
- **功能**: 创建新用户
- **参数**:
  - `username`: 用户名
  - `password`: 密码
  - `role`: 用户角色
- **返回值**: 创建成功返回true，失败返回false
- **权限要求**: 需要DBA权限
- **异常**: 不会抛出异常

##### 检查用户是否存在
```cpp
bool userExists(const std::string& username) const;
```
- **功能**: 检查用户是否存在
- **参数**: `username`: 用户名
- **返回值**: 用户存在返回true，不存在返回false
- **异常**: 不会抛出异常

##### 获取所有用户
```cpp
std::vector<UserInfo> getAllUsers() const;
```
- **功能**: 获取所有用户列表
- **返回值**: 用户信息向量
- **权限要求**: 需要DBA权限
- **异常**: 不会抛出异常

##### 获取用户列表（仅用户名）
```cpp
std::vector<std::string> listUsers() const;
```
- **功能**: 获取所有用户名列表
- **返回值**: 用户名向量
- **权限要求**: 需要DBA权限
- **异常**: 不会抛出异常

#### 权限检查接口

##### 检查权限
```cpp
bool hasPermission(Permission permission) const;
```
- **功能**: 检查当前用户是否拥有指定权限
- **参数**: `permission`: 权限枚举值
- **返回值**: 有权限返回true，无权限返回false
- **异常**: 不会抛出异常

##### 获取当前用户权限列表
```cpp
std::vector<Permission> getCurrentUserPermissions() const;
```
- **功能**: 获取当前用户的所有权限
- **返回值**: 权限向量
- **异常**: 不会抛出异常

#### 角色管理接口

##### 获取角色字符串
```cpp
std::string getCurrentUserRoleString() const;
```
- **功能**: 获取当前用户角色的字符串表示
- **返回值**: 角色字符串（"DBA", "DEVELOPER", "ANALYST"）
- **异常**: 不会抛出异常

### RoleManager

角色管理类，提供角色和权限的映射关系。

#### 角色字符串转换
```cpp
std::string roleToString(Role role) const;
```
- **功能**: 将角色枚举转换为字符串
- **参数**: `role`: 角色枚举值
- **返回值**: 角色字符串

##### 字符串转角色
```cpp
Role stringToRole(const std::string& role_str) const;
```
- **功能**: 将字符串转换为角色枚举
- **参数**: `role_str`: 角色字符串
- **返回值**: 角色枚举值

#### 权限管理
```cpp
std::vector<Permission> getRolePermissions(Role role) const;
```
- **功能**: 获取指定角色的所有权限
- **参数**: `role`: 角色枚举值
- **返回值**: 权限向量

##### 检查角色权限
```cpp
bool hasPermission(Role role, Permission permission) const;
```
- **功能**: 检查指定角色是否拥有指定权限
- **参数**:
  - `role`: 角色枚举值
  - `permission`: 权限枚举值
- **返回值**: 有权限返回true，无权限返回false

## 枚举类型

### Role（用户角色）
```cpp
enum class Role {
    DBA,        // 数据库管理员
    DEVELOPER,  // 开发者
    ANALYST     // 分析师
};
```

### Permission（权限）
```cpp
enum class Permission {
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
```

## 数据结构

### UserInfo
```cpp
struct UserInfo {
    std::string username;      // 用户名
    std::string password_hash; // 密码哈希
    Role role;                 // 用户角色
    time_t created_at;         // 创建时间
    time_t last_login;         // 最后登录时间
};
```

## 权限矩阵

| 权限 | DBA | DEVELOPER | ANALYST |
|------|-----|-----------|---------|
| CREATE_USER | ✅ | ❌ | ❌ |
| CREATE_TABLE | ✅ | ✅ | ❌ |
| SELECT | ✅ | ✅ | ✅ |
| INSERT | ✅ | ✅ | ❌ |
| UPDATE | ✅ | ✅ | ❌ |
| DELETE | ✅ | ✅ | ❌ |
| DROP_TABLE | ✅ | ✅ | ❌ |
| CREATE_INDEX | ✅ | ✅ | ❌ |
| DROP_INDEX | ✅ | ✅ | ❌ |

## 使用示例

### 基本使用
```cpp
#include "auth_service.h"
#include "storage_engine.h"
#include "catalog.h"

// 初始化
StorageEngine storage_engine("database.bin", 16);
Catalog catalog(&storage_engine);
AuthService auth_service(&storage_engine, &catalog);

// 用户登录
if (auth_service.login("root", "root")) {
    std::cout << "登录成功！" << std::endl;
    
    // 检查权限
    if (auth_service.hasPermission(Permission::CREATE_TABLE)) {
        std::cout << "当前用户有创建表的权限" << std::endl;
    }
    
    // 创建用户
    if (auth_service.createUser("newuser", "password", Role::DEVELOPER)) {
        std::cout << "用户创建成功！" << std::endl;
    }
    
    // 获取所有用户
    auto users = auth_service.getAllUsers();
    for (const auto& user : users) {
        std::cout << "用户: " << user.username 
                  << ", 角色: " << auth_service.getCurrentUserRoleString() << std::endl;
    }
}

// 登出
auth_service.logout();
```

### 错误处理
```cpp
// 所有接口都不会抛出异常，通过返回值判断成功/失败
if (!auth_service.login("invalid", "password")) {
    std::cout << "登录失败：用户名或密码错误" << std::endl;
}

if (!auth_service.createUser("existing", "pass", Role::DEVELOPER)) {
    std::cout << "创建用户失败：用户可能已存在或权限不足" << std::endl;
}
```

## 注意事项

1. **线程安全**: 当前实现不是线程安全的，如果需要在多线程环境中使用，需要添加适当的同步机制。

2. **密码安全**: 密码以哈希形式存储，但当前使用的是简单的哈希算法，生产环境建议使用更安全的哈希算法。

3. **数据持久化**: 用户数据会自动持久化到数据库文件中，重启后数据不会丢失。

4. **权限检查**: 所有需要权限的操作都会自动检查当前用户权限，无需手动检查。

5. **异常安全**: 所有接口都不会抛出异常，通过返回值判断操作是否成功。

## 测试

运行权限管理接口测试：
```bash
.\build\tests\Debug\test_auth_interfaces.exe
```

该测试程序会验证所有接口的正确性，确保不会产生报错。
