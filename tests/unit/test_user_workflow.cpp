#include "src/auth/auth_service.h"
#include "src/storage/storage_engine.h"
#include "src/catalog/catalog.h"
#include <iostream>

using namespace minidb;

int main() {
    std::cout << "=== 测试完整的用户创建和登录流程 ===" << std::endl;
    
    const std::string dbFile = "data/test_user_workflow.bin";
    
    // 第一次运行：创建root用户和新用户
    std::cout << "\n--- 第一次运行：创建用户 ---" << std::endl;
    {
        StorageEngine storage_engine(dbFile, 16);
        Catalog catalog(&storage_engine);
        AuthService auth_service(&storage_engine, &catalog);
        
        // 1. root用户登录
        std::cout << "1. Root用户登录: " << (auth_service.login("root", "root") ? "成功" : "失败") << std::endl;
        std::cout << "   Root用户角色: " << (auth_service.isDBA() ? "DBA" : "非DBA") << std::endl;
        
        // 2. 创建新用户
        std::cout << "2. 创建developer用户: " << (auth_service.createUser("alice", "password123", Role::DEVELOPER) ? "成功" : "失败") << std::endl;
        std::cout << "3. 创建analyst用户: " << (auth_service.createUser("bob", "secret456", Role::ANALYST) ? "成功" : "失败") << std::endl;
        
        // 3. 验证用户存在
        std::cout << "4. 验证用户存在:" << std::endl;
        std::cout << "   - root: " << (auth_service.userExists("root") ? "存在" : "不存在") << std::endl;
        std::cout << "   - alice: " << (auth_service.userExists("alice") ? "存在" : "不存在") << std::endl;
        std::cout << "   - bob: " << (auth_service.userExists("bob") ? "存在" : "不存在") << std::endl;
        
        std::cout << "第一次运行完成，数据已保存到 " << dbFile << std::endl;
    }
    
    // 第二次运行：模拟程序重启，验证数据持久化
    std::cout << "\n--- 第二次运行：验证数据持久化 ---" << std::endl;
    {
        StorageEngine storage_engine(dbFile, 16);
        Catalog catalog(&storage_engine);
        AuthService auth_service(&storage_engine, &catalog);
        
        // 1. 验证所有用户仍然存在
        std::cout << "1. 验证用户持久化:" << std::endl;
        std::cout << "   - root: " << (auth_service.userExists("root") ? "存在" : "不存在") << std::endl;
        std::cout << "   - alice: " << (auth_service.userExists("alice") ? "存在" : "不存在") << std::endl;
        std::cout << "   - bob: " << (auth_service.userExists("bob") ? "存在" : "不存在") << std::endl;
        
        // 2. 测试root用户登录
        std::cout << "2. Root用户登录: " << (auth_service.login("root", "root") ? "成功" : "失败") << std::endl;
        std::cout << "   Root用户角色: " << (auth_service.isDBA() ? "DBA" : "非DBA") << std::endl;
        
        // 3. 测试alice用户登录（如果存在）
        if (auth_service.userExists("alice")) {
            std::cout << "3. Alice用户登录: " << (auth_service.login("alice", "password123") ? "成功" : "失败") << std::endl;
            if (auth_service.isLoggedIn()) {
                std::cout << "   Alice用户角色: " << (auth_service.getCurrentUserRole() == Role::DEVELOPER ? "DEVELOPER" : "其他") << std::endl;
                std::cout << "   Alice可以创建表: " << (auth_service.hasPermission(Permission::CREATE_TABLE) ? "是" : "否") << std::endl;
                std::cout << "   Alice可以创建用户: " << (auth_service.hasPermission(Permission::CREATE_USER) ? "是" : "否") << std::endl;
            }
        } else {
            std::cout << "3. Alice用户不存在，跳过登录测试" << std::endl;
        }
        
        // 4. 测试bob用户登录（如果存在）
        if (auth_service.userExists("bob")) {
            std::cout << "4. Bob用户登录: " << (auth_service.login("bob", "secret456") ? "成功" : "失败") << std::endl;
            if (auth_service.isLoggedIn()) {
                std::cout << "   Bob用户角色: " << (auth_service.getCurrentUserRole() == Role::ANALYST ? "ANALYST" : "其他") << std::endl;
                std::cout << "   Bob可以查询: " << (auth_service.hasPermission(Permission::SELECT) ? "是" : "否") << std::endl;
                std::cout << "   Bob可以创建表: " << (auth_service.hasPermission(Permission::CREATE_TABLE) ? "是" : "否") << std::endl;
            }
        } else {
            std::cout << "4. Bob用户不存在，跳过登录测试" << std::endl;
        }
        
        // 5. 测试错误密码（只在用户存在时测试）
        if (auth_service.userExists("alice")) {
            std::cout << "5. 测试错误密码: " << (auth_service.login("alice", "wrongpassword") ? "成功" : "失败") << std::endl;
        } else {
            std::cout << "5. Alice用户不存在，跳过错误密码测试" << std::endl;
        }
        
        // 6. 测试不存在的用户（安全测试）
        std::cout << "6. 测试不存在的用户: " << (auth_service.login("nonexistent", "password") ? "成功" : "失败") << std::endl;
    }
    
    std::cout << "\n=== 测试完成 ===" << std::endl;
    return 0;
}
