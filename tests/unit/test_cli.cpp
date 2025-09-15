#include "src/auth/auth_service.h"
#include "src/storage/storage_engine.h"
#include "src/catalog/catalog.h"
#include <iostream>
#include <string>
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

void printHelp() {
    std::cout << "\n=== MiniDB 用户管理测试 ===" << std::endl;
    std::cout << "可用命令:" << std::endl;
    std::cout << "  login <用户名> <密码>     - 登录用户" << std::endl;
    std::cout << "  logout                  - 登出当前用户" << std::endl;
    std::cout << "  create <用户名> <密码> <角色> - 创建用户 (角色: DBA/DEVELOPER/ANALYST)" << std::endl;
    std::cout << "  list                    - 列出所有用户" << std::endl;
    std::cout << "  whoami                  - 显示当前用户" << std::endl;
    std::cout << "  permissions             - 显示当前用户权限" << std::endl;
    std::cout << "  help                    - 显示帮助" << std::endl;
    std::cout << "  exit                    - 退出程序" << std::endl;
    std::cout << std::endl;
}

void showPermissions(AuthService& auth_service) {
    if (!auth_service.isLoggedIn()) {
        std::cout << "请先登录" << std::endl;
        return;
    }
    
    std::cout << "当前用户权限:" << std::endl;
    std::cout << "  - 创建用户: " << (auth_service.hasPermission(Permission::CREATE_USER) ? "是" : "否") << std::endl;
    std::cout << "  - 创建表: " << (auth_service.hasPermission(Permission::CREATE_TABLE) ? "是" : "否") << std::endl;
    std::cout << "  - 查询: " << (auth_service.hasPermission(Permission::SELECT) ? "是" : "否") << std::endl;
    std::cout << "  - 插入: " << (auth_service.hasPermission(Permission::INSERT) ? "是" : "否") << std::endl;
    std::cout << "  - 更新: " << (auth_service.hasPermission(Permission::UPDATE) ? "是" : "否") << std::endl;
    std::cout << "  - 删除: " << (auth_service.hasPermission(Permission::DELETE) ? "是" : "否") << std::endl;
}

int main() {
    // 设置控制台中文编码
    setupConsoleEncoding();
    
    std::cout << "=== MiniDB 用户管理测试程序 ===" << std::endl;
    std::cout << "正在初始化数据库..." << std::endl;
    
    const std::string dbFile = "data/test_cli.bin";
    StorageEngine storage_engine(dbFile, 16);
    Catalog catalog(&storage_engine);
    AuthService auth_service(&storage_engine, &catalog);
    
    std::cout << "数据库初始化完成！" << std::endl;
    printHelp();
    
    std::string command;
    while (true) {
        std::cout << "minidb> ";
        std::getline(std::cin, command);
        
        if (command.empty()) continue;
        
        std::istringstream iss(command);
        std::string cmd;
        iss >> cmd;
        
        // 去除可能的BOM字符
        if (cmd.length() >= 3 && cmd.substr(0, 3) == "\xEF\xBB\xBF") {
            cmd = cmd.substr(3);
        }
        
        
        if (cmd == "exit" || cmd == "quit") {
            std::cout << "再见！" << std::endl;
            break;
        }
        else if (cmd == "help") {
            printHelp();
        }
        else if (cmd == "login") {
            std::string username, password;
            iss >> username >> password;
            if (username.empty() || password.empty()) {
                std::cout << "用法: login <用户名> <密码>" << std::endl;
                continue;
            }
            
            if (auth_service.login(username, password)) {
                std::cout << "登录成功！欢迎 " << username << std::endl;
                std::cout << "用户角色: " << (auth_service.isDBA() ? "DBA" : "普通用户") << std::endl;
            } else {
                std::cout << "登录失败！用户名或密码错误" << std::endl;
            }
        }
        else if (cmd == "logout") {
            if (auth_service.isLoggedIn()) {
                auth_service.logout();
                std::cout << "已登出" << std::endl;
            } else {
                std::cout << "当前没有登录的用户" << std::endl;
            }
        }
        else if (cmd == "create") {
            std::string username, password, role_str;
            iss >> username >> password >> role_str;
            if (username.empty() || password.empty() || role_str.empty()) {
                std::cout << "用法: create <用户名> <密码> <角色>" << std::endl;
                std::cout << "角色: DBA, DEVELOPER, ANALYST" << std::endl;
                continue;
            }
            
            Role role;
            if (role_str == "DBA") role = Role::DBA;
            else if (role_str == "DEVELOPER") role = Role::DEVELOPER;
            else if (role_str == "ANALYST") role = Role::ANALYST;
            else {
                std::cout << "无效的角色！请使用: DBA, DEVELOPER, ANALYST" << std::endl;
                continue;
            }
            
            if (auth_service.createUser(username, password, role)) {
                std::cout << "用户 " << username << " 创建成功！" << std::endl;
            } else {
                std::cout << "用户创建失败！可能用户已存在或权限不足" << std::endl;
            }
        }
        else if (cmd == "list") {
            auto users = auth_service.getAllUsers();
            if (users.empty()) {
                std::cout << "没有用户" << std::endl;
            } else {
                std::cout << "用户列表:" << std::endl;
                for (const auto& user : users) {
                    std::cout << "  - " << user.username << " (角色: " << 
                        (user.role == Role::DBA ? "DBA" : 
                         user.role == Role::DEVELOPER ? "DEVELOPER" : "ANALYST") << ")" << std::endl;
                }
            }
        }
        else if (cmd == "whoami") {
            if (auth_service.isLoggedIn()) {
                std::cout << "当前用户: " << auth_service.getCurrentUser() << std::endl;
                std::cout << "角色: " << (auth_service.isDBA() ? "DBA" : "普通用户") << std::endl;
            } else {
                std::cout << "当前没有登录的用户" << std::endl;
            }
        }
        else if (cmd == "permissions") {
            showPermissions(auth_service);
        }
        else {
            std::cout << "未知命令: " << cmd << std::endl;
            std::cout << "输入 'help' 查看可用命令" << std::endl;
        }
    }
    
    return 0;
}
