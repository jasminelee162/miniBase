# MiniBase 权限管理模块 (Auth Module)

本模块实现了MiniBase数据库系统的用户认证和权限管理功能，支持基于角色的访问控制(RBAC)。

## 功能特性

- **用户管理**：用户注册、登录、认证
- **角色管理**：DBA、DEVELOPER、ANALYST三种角色
- **权限控制**：基于角色的细粒度权限管理
- **数据持久化**：用户数据持久化存储
- **安全认证**：密码哈希和用户验证

## 角色定义

### DBA (数据库管理员)
- **权限**：所有权限
- **职责**：数据库整体管理、用户管理、系统维护
- **典型操作**：创建/删除数据库、表结构设计、备份恢复、分配权限

### DEVELOPER (开发人员)
- **权限**：数据操作和表结构权限
- **职责**：应用开发和功能实现
- **典型操作**：建表、修改表、增删改查数据

### ANALYST (数据分析人员)
- **权限**：只读权限
- **职责**：数据查询和分析
- **典型操作**：查询数据，不能修改数据或表结构

## 权限列表

| 权限 | DBA | DEVELOPER | ANALYST | 说明 |
|------|-----|-----------|---------|------|
| SELECT | ✅ | ✅ | ✅ | 查询数据 |
| INSERT | ✅ | ✅ | ❌ | 插入数据 |
| UPDATE | ✅ | ✅ | ❌ | 更新数据 |
| DELETE | ✅ | ✅ | ❌ | 删除数据 |
| CREATE_TABLE | ✅ | ✅ | ❌ | 创建表 |
| DROP_TABLE | ✅ | ❌ | ❌ | 删除表 |
| ALTER_TABLE | ✅ | ✅ | ❌ | 修改表结构 |
| CREATE_INDEX | ✅ | ✅ | ❌ | 创建索引 |
| DROP_INDEX | ✅ | ❌ | ❌ | 删除索引 |
| CREATE_USER | ✅ | ❌ | ❌ | 创建用户 |
| DROP_USER | ✅ | ❌ | ❌ | 删除用户 |
| GRANT | ✅ | ❌ | ❌ | 授予权限 |
| REVOKE | ✅ | ❌ | ❌ | 撤销权限 |

## 使用方法

### 1. 基本使用

```cpp
#include "auth_service.h"

using namespace minidb;

// 创建认证服务
AuthService auth_service;

// 用户登录
if (auth_service.login("admin", "admin123")) {
    std::cout << "Login successful!" << std::endl;
    std::cout << "Current user: " << auth_service.getCurrentUser() << std::endl;
    std::cout << "Role: " << auth_service.getCurrentUserRoleString() << std::endl;
}

// 权限检查
if (auth_service.hasPermission(Permission::CREATE_TABLE)) {
    std::cout << "User can create tables" << std::endl;
}

// 用户登出
auth_service.logout();
```

### 2. 用户管理（需要DBA权限）

```cpp
// 创建用户
auth_service.createUser("developer1", "password123", Role::DEVELOPER);
auth_service.createUser("analyst1", "password456", Role::ANALYST);

// 列出所有用户
auto users = auth_service.getAllUsers();
for (const auto& user : users) {
    std::cout << user.username << " (" << roleToString(user.role) << ")" << std::endl;
}

// 删除用户
auth_service.deleteUser("username");
```

### 3. 权限检查

```cpp
// 检查特定权限
if (auth_service.hasPermission(Permission::SELECT)) {
    // 用户可以查询数据
}

// 检查是否为DBA
if (auth_service.isDBA()) {
    // 用户是DBA
}

// 获取用户所有权限
auto permissions = auth_service.getCurrentUserPermissions();
for (const auto& perm : permissions) {
    std::cout << permissionToString(perm) << std::endl;
}
```

### 4. 数据持久化

```cpp
// 保存用户数据到文件
auth_service.saveToFile("users.dat");

// 从文件加载用户数据
auth_service.loadFromFile("users.dat");
```

## 文件结构

```
src/auth/
├── role_manager.h/cpp      # 角色管理
├── user_manager.h/cpp      # 用户管理
├── auth_service.h/cpp      # 认证服务
├── permission_checker.h/cpp # 权限检查
├── auth_test.cpp           # 测试程序
├── usage_example.cpp       # 使用示例
├── CMakeLists.txt          # 构建配置
└── README.md               # 说明文档
```

## 构建和测试

### 构建
```bash
cd build
cmake --build . --target auth_lib
```

### 运行测试
```bash
cd build
./src/auth/auth_test
```

### 运行示例
```bash
cd build
./src/auth/auth_example
```

## 默认用户

系统启动时会自动创建默认DBA用户：
- **用户名**：admin
- **密码**：admin123
- **角色**：DBA

## 安全注意事项

1. **密码存储**：当前使用简单哈希，生产环境应使用更安全的哈希算法
2. **权限验证**：每次操作前都应进行权限检查
3. **会话管理**：用户登出后应清除会话信息
4. **数据备份**：定期备份用户数据文件

## 扩展功能

后续可以添加的功能：
- 密码强度验证
- 会话超时管理
- 审计日志记录
- 更细粒度的权限控制（表级、列级）
- 角色继承和组合
- 多因素认证

## 集成说明

本模块设计为独立模块，可以轻松集成到现有系统中：

1. 在需要权限检查的地方包含 `auth_service.h`
2. 创建 `AuthService` 实例
3. 在操作前调用 `hasPermission()` 检查权限
4. 根据权限检查结果决定是否允许操作

模块不依赖其他业务模块，可以独立使用和测试。
