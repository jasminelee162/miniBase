#include "../../src/auth/auth_service.h"
#include "../../src/storage/storage_engine.h"
#include "../../src/catalog/catalog.h"
#include <iostream>
#include <cassert>

using namespace minidb;

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

// 测试重复登录安全约束
bool testDuplicateLoginSecurity() {
    std::cout << "\n=== 测试重复登录安全约束 ===" << std::endl;
    
    const std::string dbFile = "data/test_auth_security_" + std::to_string(time(nullptr)) + ".bin";
    std::remove(dbFile.c_str());
    
    StorageEngine storage_engine(dbFile, 16);
    Catalog catalog(&storage_engine);
    AuthService auth_service(&storage_engine, &catalog);
    
    // 测试1: 正常登录
    TEST_ASSERT(auth_service.login("root", "root"), "root用户登录应该成功");
    TEST_ASSERT(auth_service.isLoggedIn(), "登录后应该处于登录状态");
    TEST_ASSERT(auth_service.getCurrentUser() == "root", "当前用户应该是root");
    
    // 测试2: 尝试重复登录（应该失败）
    TEST_ASSERT(!auth_service.login("root", "root"), "已登录用户重复登录应该失败");
    TEST_ASSERT(auth_service.getCurrentUser() == "root", "重复登录失败后当前用户应该仍然是root");
    
    // 测试3: 尝试登录其他用户（应该失败）
    TEST_ASSERT(!auth_service.login("admin", "admin"), "已登录用户登录其他用户应该失败");
    TEST_ASSERT(auth_service.getCurrentUser() == "root", "登录其他用户失败后当前用户应该仍然是root");
    
    // 测试4: 登出后可以重新登录
    auth_service.logout();
    TEST_ASSERT(!auth_service.isLoggedIn(), "登出后应该未登录");
    TEST_ASSERT(auth_service.login("root", "root"), "登出后重新登录应该成功");
    
    return true;
}

// 测试非登录状态操作限制
bool testUnauthenticatedOperationSecurity() {
    std::cout << "\n=== 测试非登录状态操作限制 ===" << std::endl;
    
    const std::string dbFile = "data/test_auth_security_" + std::to_string(time(nullptr)) + ".bin";
    std::remove(dbFile.c_str());
    
    StorageEngine storage_engine(dbFile, 16);
    Catalog catalog(&storage_engine);
    AuthService auth_service(&storage_engine, &catalog);
    
    // 确保未登录状态
    TEST_ASSERT(!auth_service.isLoggedIn(), "初始状态应该未登录");
    
    // 测试1: 非登录状态创建用户（应该失败）
    TEST_ASSERT(!auth_service.createUser("testuser", "password", Role::DEVELOPER), 
                "非登录状态创建用户应该失败");
    
    // 测试2: 非登录状态删除用户（应该失败）
    TEST_ASSERT(!auth_service.deleteUser("testuser"), 
                "非登录状态删除用户应该失败");
    
    // 测试3: 非登录状态列出用户（应该返回空）
    auto users = auth_service.listUsers();
    TEST_ASSERT(users.empty(), "非登录状态列出用户应该返回空列表");
    
    // 测试4: 非登录状态获取所有用户（应该返回空）
    auto all_users = auth_service.getAllUsers();
    TEST_ASSERT(all_users.empty(), "非登录状态获取所有用户应该返回空列表");
    
    // 测试5: 非登录状态检查权限（应该返回false）
    TEST_ASSERT(!auth_service.hasPermission(Permission::CREATE_TABLE), 
                "非登录状态检查权限应该返回false");
    
    // 测试6: 非登录状态获取角色（应该返回ANALYST）
    TEST_ASSERT(auth_service.getCurrentUserRole() == Role::ANALYST, 
                "非登录状态获取角色应该返回ANALYST");
    
    return true;
}

// 测试权限检查安全约束
bool testPermissionSecurity() {
    std::cout << "\n=== 测试权限检查安全约束 ===" << std::endl;
    
    const std::string dbFile = "data/test_auth_security_" + std::to_string(time(nullptr)) + ".bin";
    std::remove(dbFile.c_str());
    
    StorageEngine storage_engine(dbFile, 16);
    Catalog catalog(&storage_engine);
    AuthService auth_service(&storage_engine, &catalog);
    
    // 测试1: DBA用户权限
    TEST_ASSERT(auth_service.login("root", "root"), "root用户登录应该成功");
    TEST_ASSERT(auth_service.createUser("devuser", "devpass", Role::DEVELOPER), 
                "DBA用户创建DEVELOPER用户应该成功");
    TEST_ASSERT(auth_service.createUser("analystuser", "analystpass", Role::ANALYST), 
                "DBA用户创建ANALYST用户应该成功");
    
    // 测试2: DEVELOPER用户权限限制
    TEST_ASSERT(auth_service.login("devuser", "devpass"), "devuser登录应该成功");
    TEST_ASSERT(!auth_service.createUser("shouldfail", "password", Role::DEVELOPER), 
                "DEVELOPER用户创建用户应该失败");
    TEST_ASSERT(!auth_service.deleteUser("analystuser"), 
                "DEVELOPER用户删除用户应该失败");
    
    // 测试3: ANALYST用户权限限制
    TEST_ASSERT(auth_service.login("analystuser", "analystpass"), "analystuser登录应该成功");
    TEST_ASSERT(!auth_service.createUser("shouldfail", "password", Role::DEVELOPER), 
                "ANALYST用户创建用户应该失败");
    TEST_ASSERT(!auth_service.deleteUser("devuser"), 
                "ANALYST用户删除用户应该失败");
    
    return true;
}

int main() {
    std::cout << "=== Auth模块安全约束测试程序 ===" << std::endl;
    std::cout << "测试认证和权限管理的安全约束" << std::endl;
    std::cout << "===============================================" << std::endl;
    
    bool all_passed = true;
    
    try {
        // 运行所有安全测试
        all_passed &= testDuplicateLoginSecurity();
        all_passed &= testUnauthenticatedOperationSecurity();
        all_passed &= testPermissionSecurity();
        
    } catch (const std::exception& e) {
        std::cout << "[ERROR] 测试过程中发生异常: " << e.what() << std::endl;
        all_passed = false;
    }
    
    std::cout << "\n===============================================" << std::endl;
    if (all_passed) {
        std::cout << "✅ 所有安全约束测试通过！" << std::endl;
        std::cout << "✅ Auth模块安全约束设计合理" << std::endl;
    } else {
        std::cout << "❌ 部分安全约束测试失败，需要修复问题" << std::endl;
    }
    
    return all_passed ? 0 : 1;
}
