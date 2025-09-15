#include "src/auth/auth_service.h"
#include "src/storage/storage_engine.h"
#include "src/catalog/catalog.h"
#include <iostream>

using namespace minidb;

int main() {
    std::cout << "=== 调试测试程序 ===" << std::endl;
    
    try {
        const std::string dbFile = "data/test_debug.bin";
        
        std::cout << "1. 创建存储引擎..." << std::endl;
        StorageEngine storage_engine(dbFile, 16);
        
        std::cout << "2. 创建目录..." << std::endl;
        Catalog catalog(&storage_engine);
        
        std::cout << "3. 创建认证服务..." << std::endl;
        AuthService auth_service(&storage_engine, &catalog);
        
        std::cout << "4. 测试root用户登录..." << std::endl;
        bool login_result = auth_service.login("root", "root");
        std::cout << "   Root用户登录: " << (login_result ? "成功" : "失败") << std::endl;
        
        if (login_result) {
            std::cout << "5. 创建alice用户..." << std::endl;
            bool create_alice = auth_service.createUser("alice", "password123", Role::DEVELOPER);
            std::cout << "   创建alice: " << (create_alice ? "成功" : "失败") << std::endl;
            
            std::cout << "6. 创建bob用户..." << std::endl;
            bool create_bob = auth_service.createUser("bob", "secret456", Role::ANALYST);
            std::cout << "   创建bob: " << (create_bob ? "成功" : "失败") << std::endl;
            
            std::cout << "7. 登出root用户..." << std::endl;
            auth_service.logout();
            
            std::cout << "8. 测试alice用户登录..." << std::endl;
            bool alice_login = auth_service.login("alice", "password123");
            std::cout << "   Alice登录: " << (alice_login ? "成功" : "失败") << std::endl;
            
            if (alice_login) {
                std::cout << "9. 测试alice权限..." << std::endl;
                std::cout << "   Alice可以创建表: " << (auth_service.hasPermission(Permission::CREATE_TABLE) ? "是" : "否") << std::endl;
                std::cout << "   Alice可以创建用户: " << (auth_service.hasPermission(Permission::CREATE_USER) ? "是" : "否") << std::endl;
            }
            
            std::cout << "10. 登出alice用户..." << std::endl;
            auth_service.logout();
            
            std::cout << "11. 测试bob用户登录..." << std::endl;
            bool bob_login = auth_service.login("bob", "secret456");
            std::cout << "   Bob登录: " << (bob_login ? "成功" : "失败") << std::endl;
            
            if (bob_login) {
                std::cout << "12. 测试bob权限..." << std::endl;
                std::cout << "   Bob可以查询: " << (auth_service.hasPermission(Permission::SELECT) ? "是" : "否") << std::endl;
                std::cout << "   Bob可以创建表: " << (auth_service.hasPermission(Permission::CREATE_TABLE) ? "是" : "否") << std::endl;
            }
        }
        
        std::cout << "=== 第一次运行完成 ===" << std::endl;
        
        // 第二次运行：测试持久化
        std::cout << "\n=== 第二次运行：测试持久化 ===" << std::endl;
        
        std::cout << "13. 重新创建存储引擎..." << std::endl;
        StorageEngine storage_engine2(dbFile, 16);
        Catalog catalog2(&storage_engine2);
        AuthService auth_service2(&storage_engine2, &catalog2);
        
        std::cout << "14. 测试root用户登录..." << std::endl;
        bool root_login2 = auth_service2.login("root", "root");
        std::cout << "   Root登录: " << (root_login2 ? "成功" : "失败") << std::endl;
        
        std::cout << "15. 测试alice用户登录..." << std::endl;
        bool alice_login2 = auth_service2.login("alice", "password123");
        std::cout << "   Alice登录: " << (alice_login2 ? "成功" : "失败") << std::endl;
        
        std::cout << "16. 测试bob用户登录..." << std::endl;
        bool bob_login2 = auth_service2.login("bob", "secret456");
        std::cout << "   Bob登录: " << (bob_login2 ? "成功" : "失败") << std::endl;
        
        std::cout << "=== 测试完成 ===" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "[ERROR] 异常: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cout << "[ERROR] 未知异常" << std::endl;
        return 1;
    }
    
    return 0;
}
