#include "../../src/auth/auth_service.h"
#include "../../src/storage/storage_engine.h"
#include "../../src/catalog/catalog.h"
#include "../../src/auth/role_manager.h"
#include <iostream>
#include <fstream>
#include <cassert>
#include <windows.h>
#include <io.h>
#include <fcntl.h>

// 防止Windows宏定义冲突
#ifdef DELETE
#undef DELETE
#endif

using namespace minidb;

// 设置控制台中文编码
void setupConsoleEncoding() {
    // 设置控制台代码页为UTF-8
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
}

// 测试辅助宏
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            std::cout << "[FAIL] " << message << std::endl; \
            return false; \
        } else { \
            std::cout << "[PASS] " << message << std::endl; \
        } \
    } while(0)

// 测试用户认证接口
bool testAuthenticationInterfaces() {
    std::cout << "\n=== 测试用户认证接口 ===" << std::endl;
    
    const std::string dbFile = "data/test_auth_interfaces.bin";
    StorageEngine storage_engine(dbFile, 16);
    Catalog catalog(&storage_engine);
    AuthService auth_service(&storage_engine, &catalog);
    
    // 测试1: 初始状态检查
    TEST_ASSERT(!auth_service.isLoggedIn(), "初始状态应该未登录");
    TEST_ASSERT(auth_service.getCurrentUser().empty(), "初始状态当前用户应该为空");
    TEST_ASSERT(auth_service.getCurrentUserRole() == Role::ANALYST, "初始状态角色应该是ANALYST");
    
    // 测试2: 登录功能
    TEST_ASSERT(auth_service.login("root", "root"), "root用户登录应该成功");
    TEST_ASSERT(auth_service.isLoggedIn(), "登录后应该处于登录状态");
    TEST_ASSERT(auth_service.getCurrentUser() == "root", "当前用户应该是root");
    TEST_ASSERT(auth_service.getCurrentUserRole() == Role::DBA, "root用户角色应该是DBA");
    TEST_ASSERT(auth_service.isDBA(), "root用户应该是DBA");
    
    // 测试3: 登出功能
    auth_service.logout();
    TEST_ASSERT(!auth_service.isLoggedIn(), "登出后应该未登录");
    TEST_ASSERT(auth_service.getCurrentUser().empty(), "登出后当前用户应该为空");
    
    return true;
}

// 测试用户管理接口
bool testUserManagementInterfaces() {
    std::cout << "\n=== 测试用户管理接口 ===" << std::endl;
    
    const std::string dbFile = "data/test_user_mgmt_" + std::to_string(time(nullptr)) + ".bin";
    // 确保删除之前的测试文件
    std::remove(dbFile.c_str());
    
    StorageEngine storage_engine(dbFile, 16);
    Catalog catalog(&storage_engine);
    AuthService auth_service(&storage_engine, &catalog);
    
    // 先登录为DBA用户
    TEST_ASSERT(auth_service.login("root", "root"), "root用户登录应该成功");
    
    // 测试1: 创建用户
    TEST_ASSERT(auth_service.createUser("testuser1", "password123", Role::DEVELOPER), "创建DEVELOPER用户应该成功");
    TEST_ASSERT(auth_service.createUser("testuser2", "password456", Role::ANALYST), "创建ANALYST用户应该成功");
    
    // 测试2: 用户存在检查
    TEST_ASSERT(auth_service.userExists("testuser1"), "testuser1用户应该存在");
    TEST_ASSERT(auth_service.userExists("testuser2"), "testuser2用户应该存在");
    TEST_ASSERT(!auth_service.userExists("nonexistent"), "不存在的用户应该返回false");
    
    // 测试3: 获取所有用户
    auto users = auth_service.getAllUsers();
    TEST_ASSERT(users.size() >= 3, "应该至少有3个用户(root, testuser1, testuser2)");
    
    // 验证用户信息
    bool found_root = false, found_user1 = false, found_user2 = false;
    for (const auto& user : users) {
        if (user.username == "root" && user.role == Role::DBA) found_root = true;
        if (user.username == "testuser1" && user.role == Role::DEVELOPER) found_user1 = true;
        if (user.username == "testuser2" && user.role == Role::ANALYST) found_user2 = true;
    }
    TEST_ASSERT(found_root, "应该找到root用户");
    TEST_ASSERT(found_user1, "应该找到testuser1用户");
    TEST_ASSERT(found_user2, "应该找到testuser2用户");
    
    return true;
}

// 测试权限检查接口
bool testPermissionInterfaces() {
    std::cout << "\n=== 测试权限检查接口 ===" << std::endl;
    
    const std::string dbFile = "data/test_permissions_" + std::to_string(time(nullptr)) + ".bin";
    // 确保删除之前的测试文件
    std::remove(dbFile.c_str());
    
    StorageEngine storage_engine(dbFile, 16);
    Catalog catalog(&storage_engine);
    AuthService auth_service(&storage_engine, &catalog);
    
    // 测试1: DBA权限
    TEST_ASSERT(auth_service.login("root", "root"), "root用户登录应该成功");
    
    // DBA应该拥有所有权限
    TEST_ASSERT(auth_service.hasPermission(Permission::CREATE_USER), "DBA应该有创建用户权限");
    TEST_ASSERT(auth_service.hasPermission(Permission::CREATE_TABLE), "DBA应该有创建表权限");
    TEST_ASSERT(auth_service.hasPermission(Permission::SELECT), "DBA应该有查询权限");
    TEST_ASSERT(auth_service.hasPermission(Permission::INSERT), "DBA应该有插入权限");
    TEST_ASSERT(auth_service.hasPermission(Permission::UPDATE), "DBA应该有更新权限");
    TEST_ASSERT(auth_service.hasPermission(Permission::DELETE), "DBA应该有删除权限");
    
    // 测试2: DEVELOPER权限
    TEST_ASSERT(auth_service.createUser("devuser", "devpass", Role::DEVELOPER), "创建DEVELOPER用户应该成功");
    auth_service.logout(); // 先登出root用户
    TEST_ASSERT(auth_service.login("devuser", "devpass"), "devuser登录应该成功");
    
    TEST_ASSERT(!auth_service.hasPermission(Permission::CREATE_USER), "DEVELOPER不应该有创建用户权限");
    TEST_ASSERT(auth_service.hasPermission(Permission::CREATE_TABLE), "DEVELOPER应该有创建表权限");
    TEST_ASSERT(auth_service.hasPermission(Permission::SELECT), "DEVELOPER应该有查询权限");
    TEST_ASSERT(auth_service.hasPermission(Permission::INSERT), "DEVELOPER应该有插入权限");
    TEST_ASSERT(auth_service.hasPermission(Permission::UPDATE), "DEVELOPER应该有更新权限");
    TEST_ASSERT(auth_service.hasPermission(Permission::DELETE), "DEVELOPER应该有删除权限");
    
    // 测试3: ANALYST权限
    // 需要先登录为DBA来创建ANALYST用户
    auth_service.logout(); // 先登出devuser
    TEST_ASSERT(auth_service.login("root", "root"), "重新登录为root用户应该成功");
    TEST_ASSERT(auth_service.createUser("analystuser", "analystpass", Role::ANALYST), "创建ANALYST用户应该成功");
    auth_service.logout(); // 先登出root用户
    TEST_ASSERT(auth_service.login("analystuser", "analystpass"), "analystuser登录应该成功");
    
    TEST_ASSERT(!auth_service.hasPermission(Permission::CREATE_USER), "ANALYST不应该有创建用户权限");
    TEST_ASSERT(!auth_service.hasPermission(Permission::CREATE_TABLE), "ANALYST不应该有创建表权限");
    TEST_ASSERT(auth_service.hasPermission(Permission::SELECT), "ANALYST应该有查询权限");
    TEST_ASSERT(!auth_service.hasPermission(Permission::INSERT), "ANALYST不应该有插入权限");
    TEST_ASSERT(!auth_service.hasPermission(Permission::UPDATE), "ANALYST不应该有更新权限");
    TEST_ASSERT(!auth_service.hasPermission(Permission::DELETE), "ANALYST不应该有删除权限");
    
    return true;
}

// 测试角色管理接口
bool testRoleInterfaces() {
    std::cout << "\n=== 测试角色管理接口 ===" << std::endl;
    
    const std::string dbFile = "data/test_roles.bin";
    StorageEngine storage_engine(dbFile, 16);
    Catalog catalog(&storage_engine);
    AuthService auth_service(&storage_engine, &catalog);
    
    // 测试1: 角色字符串转换
    RoleManager role_manager;
    TEST_ASSERT(role_manager.roleToString(Role::DBA) == "DBA", "DBA角色字符串转换应该正确");
    TEST_ASSERT(role_manager.roleToString(Role::DEVELOPER) == "DEVELOPER", "DEVELOPER角色字符串转换应该正确");
    TEST_ASSERT(role_manager.roleToString(Role::ANALYST) == "ANALYST", "ANALYST角色字符串转换应该正确");
    
    // 测试2: 角色权限检查
    auto dba_perms = role_manager.getRolePermissions(Role::DBA);
    auto dev_perms = role_manager.getRolePermissions(Role::DEVELOPER);
    auto analyst_perms = role_manager.getRolePermissions(Role::ANALYST);
    
    TEST_ASSERT(dba_perms.size() > dev_perms.size(), "DBA权限应该比DEVELOPER多");
    TEST_ASSERT(dev_perms.size() > analyst_perms.size(), "DEVELOPER权限应该比ANALYST多");
    
    // 测试3: 权限检查
    TEST_ASSERT(role_manager.hasPermission(Role::DBA, Permission::CREATE_USER), "DBA应该有创建用户权限");
    TEST_ASSERT(!role_manager.hasPermission(Role::DEVELOPER, Permission::CREATE_USER), "DEVELOPER不应该有创建用户权限");
    TEST_ASSERT(!role_manager.hasPermission(Role::ANALYST, Permission::CREATE_USER), "ANALYST不应该有创建用户权限");
    
    return true;
}

// 测试数据持久化接口
bool testPersistenceInterfaces() {
    std::cout << "\n=== 测试数据持久化接口 ===" << std::endl;
    
    const std::string dbFile = "data/test_persistence_" + std::to_string(time(nullptr)) + ".bin";
    // 确保删除之前的测试文件
    std::remove(dbFile.c_str());
    
    // 第一阶段：创建数据
    {
        StorageEngine storage_engine(dbFile, 16);
        Catalog catalog(&storage_engine);
        AuthService auth_service(&storage_engine, &catalog);
        
        TEST_ASSERT(auth_service.login("root", "root"), "root用户登录应该成功");
        TEST_ASSERT(auth_service.createUser("persistuser1", "pass1", Role::DEVELOPER), "创建用户1应该成功");
        TEST_ASSERT(auth_service.createUser("persistuser2", "pass2", Role::ANALYST), "创建用户2应该成功");
    }
    
    // 第二阶段：重新加载数据
    {
        StorageEngine storage_engine(dbFile, 16);
        Catalog catalog(&storage_engine);
        AuthService auth_service(&storage_engine, &catalog);
        
        // 验证用户数据持久化
        TEST_ASSERT(auth_service.userExists("persistuser1"), "persistuser1应该存在");
        TEST_ASSERT(auth_service.userExists("persistuser2"), "persistuser2应该存在");
        
        // 验证登录功能
        TEST_ASSERT(auth_service.login("persistuser1", "pass1"), "persistuser1登录应该成功");
        TEST_ASSERT(auth_service.getCurrentUserRole() == Role::DEVELOPER, "persistuser1角色应该是DEVELOPER");
        
        auth_service.logout(); // 先登出persistuser1
        TEST_ASSERT(auth_service.login("persistuser2", "pass2"), "persistuser2登录应该成功");
        TEST_ASSERT(auth_service.getCurrentUserRole() == Role::ANALYST, "persistuser2角色应该是ANALYST");
    }
    
    return true;
}

// 测试错误处理接口
bool testErrorHandlingInterfaces() {
    std::cout << "\n=== 测试错误处理接口 ===" << std::endl;
    
    const std::string dbFile = "data/test_errors_" + std::to_string(time(nullptr)) + ".bin";
    // 确保删除之前的测试文件
    std::remove(dbFile.c_str());
    
    StorageEngine storage_engine(dbFile, 16);
    Catalog catalog(&storage_engine);
    AuthService auth_service(&storage_engine, &catalog);
    
    // 测试1: 无效登录
    TEST_ASSERT(!auth_service.login("nonexistent", "password"), "不存在的用户登录应该失败");
    TEST_ASSERT(!auth_service.login("root", "wrongpassword"), "错误密码登录应该失败");
    
    // 测试2: 重复创建用户
    TEST_ASSERT(auth_service.login("root", "root"), "root用户登录应该成功");
    TEST_ASSERT(auth_service.createUser("duplicate", "pass", Role::DEVELOPER), "创建用户应该成功");
    TEST_ASSERT(!auth_service.createUser("duplicate", "pass", Role::DEVELOPER), "重复创建用户应该失败");
    
    // 测试3: 权限不足
    TEST_ASSERT(auth_service.createUser("limiteduser", "pass", Role::ANALYST), "创建ANALYST用户应该成功");
    auth_service.logout(); // 先登出root用户
    TEST_ASSERT(auth_service.login("limiteduser", "pass"), "limiteduser登录应该成功");
    
    // ANALYST用户不应该能创建其他用户
    TEST_ASSERT(!auth_service.createUser("shouldfail", "pass", Role::DEVELOPER), "ANALYST创建用户应该失败");
    
    return true;
}

int main() {
    // 设置控制台中文编码
    setupConsoleEncoding();
    
    std::cout << "=== 权限管理接口测试程序 ===" << std::endl;
    std::cout << "测试所有权限管理相关接口，确保不会产生报错" << std::endl;
    std::cout << "===============================================" << std::endl;
    
    bool all_passed = true;
    
    try {
        // 运行所有测试
        all_passed &= testAuthenticationInterfaces();
        all_passed &= testUserManagementInterfaces();
        all_passed &= testPermissionInterfaces();
        all_passed &= testRoleInterfaces();
        all_passed &= testPersistenceInterfaces();
        all_passed &= testErrorHandlingInterfaces();
        
    } catch (const std::exception& e) {
        std::cout << "[ERROR] 测试过程中发生异常: " << e.what() << std::endl;
        all_passed = false;
    }
    
    std::cout << "\n===============================================" << std::endl;
    if (all_passed) {
        std::cout << "✅ 所有权限管理接口测试通过！" << std::endl;
        std::cout << "✅ 权限管理模块可以安全地提供给CLI使用" << std::endl;
    } else {
        std::cout << "❌ 部分测试失败，需要修复问题" << std::endl;
    }
    
    return all_passed ? 0 : 1;
}
