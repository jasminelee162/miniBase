#include "src/auth/auth_service.h"
#include "src/storage/storage_engine.h"
#include "src/catalog/catalog.h"
#include <iostream>

using namespace minidb;

int main() {
    std::cout << "=== 简单测试程序 ===" << std::endl;
    
    try {
        const std::string dbFile = "data/test_simple.bin";
        
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
            std::cout << "5. 测试用户角色..." << std::endl;
            std::cout << "   Root用户是DBA: " << (auth_service.isDBA() ? "是" : "否") << std::endl;
            
            std::cout << "6. 测试创建新用户..." << std::endl;
            bool create_result = auth_service.createUser("testuser", "testpass", Role::DEVELOPER);
            std::cout << "   创建testuser: " << (create_result ? "成功" : "失败") << std::endl;
            
            if (create_result) {
                std::cout << "7. 测试新用户登录..." << std::endl;
                auth_service.logout();
                bool new_login = auth_service.login("testuser", "testpass");
                std::cout << "   testuser登录: " << (new_login ? "成功" : "失败") << std::endl;
            }
        }
        
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
