#include "../../src/auth/auth_service.h"
#include "../../src/auth/permission_checker.h"
#include <iostream>
#include <string>

using namespace minidb;

// 测试辅助宏，避免使用assert()导致abort()
#define TEST_ASSERT(condition, message)                     \
    do                                                      \
    {                                                       \
        if (!(condition))                                   \
        {                                                   \
            std::cout << "[FAIL] " << message << std::endl; \
            return false;                                   \
        }                                                   \
    } while (0)

#define TEST_ASSERT_CONTINUE(condition, message)            \
    do                                                      \
    {                                                       \
        if (!(condition))                                   \
        {                                                   \
            std::cout << "[FAIL] " << message << std::endl; \
            return;                                         \
        }                                                   \
    } while (0)

bool test_role_manager()
{
    std::cout << "=== Testing RoleManager ===" << std::endl;

    RoleManager role_manager;

    // 测试角色转换
    TEST_ASSERT(role_manager.roleToString(Role::DBA) == "DBA", "DBA role string conversion failed");
    TEST_ASSERT(role_manager.roleToString(Role::DEVELOPER) == "DEVELOPER", "DEVELOPER role string conversion failed");
    TEST_ASSERT(role_manager.roleToString(Role::ANALYST) == "ANALYST", "ANALYST role string conversion failed");

    // 测试字符串转角色
    TEST_ASSERT(role_manager.stringToRole("DBA") == Role::DBA, "String to DBA role conversion failed");
    TEST_ASSERT(role_manager.stringToRole("DEVELOPER") == Role::DEVELOPER, "String to DEVELOPER role conversion failed");
    TEST_ASSERT(role_manager.stringToRole("ANALYST") == Role::ANALYST, "String to ANALYST role conversion failed");

    // 测试权限检查
    TEST_ASSERT(role_manager.hasPermission(Role::DBA, Permission::CREATE_TABLE) == true, "DBA should have CREATE_TABLE permission");
    TEST_ASSERT(role_manager.hasPermission(Role::DBA, Permission::SELECT) == true, "DBA should have SELECT permission");
    TEST_ASSERT(role_manager.hasPermission(Role::DEVELOPER, Permission::CREATE_TABLE) == true, "DEVELOPER should have CREATE_TABLE permission");
    TEST_ASSERT(role_manager.hasPermission(Role::DEVELOPER, Permission::CREATE_USER) == false, "DEVELOPER should not have CREATE_USER permission");
    TEST_ASSERT(role_manager.hasPermission(Role::ANALYST, Permission::SELECT) == true, "ANALYST should have SELECT permission");
    TEST_ASSERT(role_manager.hasPermission(Role::ANALYST, Permission::INSERT) == false, "ANALYST should not have INSERT permission");

    std::cout << "[OK] RoleManager tests passed!" << std::endl;
    return true;
}

bool test_user_manager()
{
    std::cout << "=== Testing UserManager ===" << std::endl;

    UserManager user_manager;

    // 测试创建用户
    bool create_result = user_manager.createUser("test_user", "password123", Role::DEVELOPER);
    std::cout << "Create user result: " << create_result << std::endl;
    TEST_ASSERT(create_result == true, "Failed to create test user");
    TEST_ASSERT(user_manager.userExists("test_user") == true, "Test user should exist");
    TEST_ASSERT(user_manager.userExists("nonexistent") == false, "Non-existent user should not exist");

    // 测试认证
    bool auth_result = user_manager.authenticate("test_user", "password123");
    std::cout << "Authentication result: " << auth_result << std::endl;
    TEST_ASSERT(auth_result == true, "Valid authentication should succeed");
    TEST_ASSERT(user_manager.authenticate("test_user", "wrong_password") == false, "Invalid password should fail");
    TEST_ASSERT(user_manager.authenticate("nonexistent", "password123") == false, "Non-existent user should fail");

    // 测试权限检查
    TEST_ASSERT(user_manager.hasPermission("test_user", Permission::CREATE_TABLE) == true, "Test user should have CREATE_TABLE permission");
    TEST_ASSERT(user_manager.hasPermission("test_user", Permission::CREATE_USER) == false, "Test user should not have CREATE_USER permission");
    TEST_ASSERT(user_manager.getUserRole("test_user") == Role::DEVELOPER, "Test user should have DEVELOPER role");
    TEST_ASSERT(user_manager.isDBA("test_user") == false, "Test user should not be DBA");

    // 测试DBA用户
    TEST_ASSERT(user_manager.isDBA("admin") == true, "Admin should be DBA");
    TEST_ASSERT(user_manager.hasPermission("admin", Permission::CREATE_USER) == true, "Admin should have CREATE_USER permission");

    std::cout << "[OK] UserManager tests passed!" << std::endl;
    return true;
}

bool test_auth_service()
{
    std::cout << "=== Testing AuthService ===" << std::endl;

    AuthService auth_service;

    // 测试登录
    bool login_result = auth_service.login("admin", "admin123");
    std::cout << "Login result: " << login_result << std::endl;
    TEST_ASSERT(login_result == true, "Admin login should succeed");
    TEST_ASSERT(auth_service.isLoggedIn() == true, "Should be logged in after successful login");
    TEST_ASSERT(auth_service.getCurrentUser() == "admin", "Current user should be admin");
    TEST_ASSERT(auth_service.isDBA() == true, "Admin should be DBA");

    // 测试权限检查
    TEST_ASSERT(auth_service.hasPermission(Permission::CREATE_USER) == true, "Admin should have CREATE_USER permission");
    TEST_ASSERT(auth_service.hasPermission(Permission::SELECT) == true, "Admin should have SELECT permission");

    // 测试登出
    auth_service.logout();
    TEST_ASSERT(auth_service.isLoggedIn() == false, "Should not be logged in after logout");
    TEST_ASSERT(auth_service.getCurrentUser() == "", "Current user should be empty after logout");

    // 测试创建用户（需要DBA权限）
    auth_service.login("admin", "admin123");
    bool create_user_result = auth_service.createUser("new_user", "password456", Role::ANALYST);
    std::cout << "Create new user result: " << create_user_result << std::endl;
    TEST_ASSERT(create_user_result == true, "Should be able to create new user as DBA");
    TEST_ASSERT(auth_service.userExists("new_user") == true, "New user should exist");

    // 测试非DBA用户创建用户
    auth_service.logout();
    auth_service.login("new_user", "password456");
    bool create_another_result = auth_service.createUser("another_user", "password789", Role::DEVELOPER);
    std::cout << "Create another user result (as non-DBA): " << create_another_result << std::endl;
    TEST_ASSERT(create_another_result == false, "Non-DBA user should not be able to create users");

    std::cout << "[OK] AuthService tests passed!" << std::endl;
    return true;
}

bool test_permission_checker()
{
    std::cout << "=== Testing PermissionChecker ===" << std::endl;

    AuthService auth_service;
    bool login_result = auth_service.login("admin", "admin123");
    std::cout << "Login for permission checker: " << login_result << std::endl;
    TEST_ASSERT(login_result == true, "Should be able to login for permission checker test");

    PermissionChecker checker(&auth_service);

    // 测试权限检查
    TEST_ASSERT(checker.checkPermission(Permission::CREATE_TABLE) == true, "Admin should have CREATE_TABLE permission");
    TEST_ASSERT(checker.checkPermission(Permission::SELECT) == true, "Admin should have SELECT permission");
    TEST_ASSERT(checker.checkPermission(Permission::CREATE_USER) == true, "Admin should have CREATE_USER permission");

    // 测试SQL权限检查
    TEST_ASSERT(checker.checkSQLPermission("SELECT * FROM users") == true, "Admin should be able to SELECT");
    TEST_ASSERT(checker.checkSQLPermission("INSERT INTO users VALUES (1, 'test')") == true, "Admin should be able to INSERT");
    TEST_ASSERT(checker.checkSQLPermission("CREATE TABLE test (id INT)") == true, "Admin should be able to CREATE TABLE");

    std::cout << "[OK] PermissionChecker tests passed!" << std::endl;
    return true;
}

int main()
{
    std::cout << "Starting Auth Module Tests..." << std::endl;
    std::cout << "=================================" << std::endl;

    bool all_passed = true;

    try
    {
        if (!test_role_manager())
        {
            all_passed = false;
        }

        if (!test_user_manager())
        {
            all_passed = false;
        }

        if (!test_auth_service())
        {
            all_passed = false;
        }

        if (!test_permission_checker())
        {
            all_passed = false;
        }

        std::cout << "=================================" << std::endl;
        if (all_passed)
        {
            std::cout << "=== All tests passed! ===" << std::endl;
            return 0;
        }
        else
        {
            std::cout << "=== Some tests failed! ===" << std::endl;
            return 1;
        }
    }
    catch (const std::exception &e)
    {
        std::cout << "[ERROR] Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}