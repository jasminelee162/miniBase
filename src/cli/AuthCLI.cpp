#include "AuthCLI.h"
#include <iostream>
#include <sstream>
#include <iomanip>

namespace minidb
{

AuthCLI::AuthCLI() : is_authenticated_(false), current_user_("") {
    initializeDatabase();
}

void AuthCLI::initializeDatabase() {
    // 初始化存储引擎和目录
    storage_engine_ = std::make_unique<StorageEngine>("data/minidb.bin", 16);
    catalog_ = std::make_unique<Catalog>(storage_engine_.get());
    
    // 尝试从存储加载目录信息
    try {
        catalog_->LoadFromStorage();
        std::cout << "[AuthCLI] 从现有数据库加载完成" << std::endl;
    } catch (...) {
        std::cout << "[AuthCLI] 创建新数据库" << std::endl;
    }

    auth_service_ = std::make_unique<AuthService>(storage_engine_.get(), catalog_.get());
    std::cout << "[AuthCLI] 数据库初始化完成" << std::endl;
}

bool AuthCLI::login(const std::string &username, const std::string &password)
{
    if (auth_service_->login(username, password))
    {
        is_authenticated_ = true;
        current_user_ = username;
        std::cout << "登录成功! 欢迎, " << username << std::endl;
        return true;
    }
    else
    {
        std::cout << "登录失败! 用户名或密码错误。" << std::endl;
        return false;
    }
}

void AuthCLI::logout()
{
    if (is_authenticated_)
    {
        auth_service_->logout();
        is_authenticated_ = false;
        current_user_ = "";
        std::cout << "已退出登录。" << std::endl;
    }
}

bool AuthCLI::isAuthenticated() const
{
    return is_authenticated_;
}

std::string AuthCLI::getCurrentUser() const
{
    return current_user_;
}

std::string AuthCLI::getCurrentUserRole() const
{
    if (!is_authenticated_)
        return "未登录";
    
    RoleManager role_manager;
    return role_manager.roleToString(auth_service_->getCurrentUserRole());
}

void AuthCLI::run()
{
    showWelcomeMessage();
    
    // 登录循环
    while (!is_authenticated_)
    {
        showLoginPrompt();
        std::string username, password;
        
        std::cout << "用户名: ";
        std::getline(std::cin, username);
        
        std::cout << "密码: ";
        std::getline(std::cin, password);
        
        if (login(username, password))
        {
            break;
        }
    }
    
    // 主菜单循环
    showMainMenu();
}

void AuthCLI::showWelcomeMessage()
{
    std::cout << "===============================================" << std::endl;
    std::cout << "        欢迎使用 MiniDB 数据库系统" << std::endl;
    std::cout << "===============================================" << std::endl;
    std::cout << "这是一个基于角色的访问控制数据库系统" << std::endl;
    std::cout << "支持 DBA、DEVELOPER、ANALYST 三种角色" << std::endl;
    std::cout << "===============================================" << std::endl;
}

void AuthCLI::showLoginPrompt()
{
    std::cout << "\n请登录以继续..." << std::endl;
}

void AuthCLI::showMainMenu()
{
    std::cout << "\n===============================================" << std::endl;
    std::cout << "主菜单 - 当前用户: " << current_user_ << " (" << getCurrentUserRole() << ")" << std::endl;
    std::cout << "===============================================" << std::endl;
    std::cout << "可用命令:" << std::endl;
    std::cout << "  .help           - 显示帮助信息" << std::endl;
    std::cout << "  .tables         - 显示表列表" << std::endl;
    std::cout << "  .users          - 管理用户 (仅管理员)" << std::endl;
    std::cout << "  .exit           - 退出系统" << std::endl;
    std::cout << "  SQL语句          - 执行SQL命令" << std::endl;
}

void AuthCLI::showHelp()
{
    std::cout << "\n=== 帮助信息 ===" << std::endl;
    std::cout << "系统命令:" << std::endl;
    std::cout << "  .help           - 显示此帮助信息" << std::endl;
    std::cout << "  .tables         - 显示数据库表列表" << std::endl;
    std::cout << "  .users          - 管理用户 (仅管理员)" << std::endl;
    std::cout << "  .exit           - 退出系统" << std::endl;
    std::cout << "  SQL语句          - 执行SQL命令" << std::endl;
    std::cout << "\n角色权限:" << std::endl;
    std::cout << "  DBA: 完全权限，可管理用户和所有表" << std::endl;
    std::cout << "  DEVELOPER: 可创建表，只能操作自己的表" << std::endl;
    std::cout << "  ANALYST: 只能查看所有表" << std::endl;
}

void AuthCLI::showTables() {
    if (!checkPermission(Permission::SELECT)) {
        showPermissionDenied("查看表");
        return;
    }
    
    std::cout << "\n--- 数据库表列表 ---" << std::endl;
    
    try {
        // 获取所有表名
        auto all_tables = catalog_->GetAllTableNames();
        
        if (all_tables.empty()) {
            std::cout << "数据库中没有表" << std::endl;
            return;
        }
        
        // 过滤掉用户表，只显示业务表
        std::vector<std::string> visible_tables;
        for (const auto& table_name : all_tables) {
            // 隐藏用户表（__users__）
            if (table_name != "__users__") {
                visible_tables.push_back(table_name);
            }
        }
        
        if (visible_tables.empty()) {
            std::cout << "没有可见的表（所有表都是系统表）" << std::endl;
            return;
        }
        
        // 显示表列表
        for (size_t i = 0; i < visible_tables.size(); ++i) {
            const auto& table_name = visible_tables[i];
            std::cout << (i + 1) << ". " << table_name;
            
            // 显示表所有者信息
            std::string owner = catalog_->GetTableOwner(table_name);
            if (!owner.empty()) {
                std::cout << " (所有者: " << owner << ")";
            } else {
                std::cout << " (系统表)";
            }
            
            std::cout << std::endl;
        }
        
        std::cout << "\n共 " << visible_tables.size() << " 张表" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "获取表列表失败: " << e.what() << std::endl;
    }
}

void AuthCLI::createTable(const std::string& table_name) {
    if (!checkPermission(Permission::CREATE_TABLE)) {
        showPermissionDenied("创建表");
        return;
    }
    
    std::cout << "创建表功能正在开发中..." << std::endl;
}

void AuthCLI::insertData(const std::string& table_name) {
    if (!checkPermission(Permission::INSERT)) {
        showPermissionDenied("插入数据");
        return;
    }
    
    std::cout << "插入数据功能正在开发中..." << std::endl;
}

void AuthCLI::selectData(const std::string& table_name) {
    if (!checkPermission(Permission::SELECT)) {
        showPermissionDenied("查询数据");
        return;
    }
    
    std::cout << "查询数据功能正在开发中..." << std::endl;
}

void AuthCLI::deleteData(const std::string& table_name) {
    if (!checkPermission(Permission::DELETE)) {
        showPermissionDenied("删除数据");
        return;
    }
    
    std::cout << "删除数据功能正在开发中..." << std::endl;
}

void AuthCLI::manageUsers() {
    if (!checkPermission(Permission::CREATE_USER)) {
        showPermissionDenied("管理用户");
        return;
    }

    std::cout << "\n=== 用户管理 ===" << std::endl;
    std::cout << "1. 创建用户" << std::endl;
    std::cout << "2. 删除用户" << std::endl;
    std::cout << "3. 列出用户" << std::endl;
    std::cout << "4. 返回主菜单" << std::endl;
    std::cout << "请选择操作: ";

    std::string choice;
    std::getline(std::cin, choice);

    if (choice == "1") {
        std::cout << "请输入用户名: ";
        std::string username;
        std::getline(std::cin, username);

        std::cout << "请输入密码: ";
        std::string password;
        std::getline(std::cin, password);

        std::cout << "请选择角色 (1: DBA, 2: DEVELOPER, 3: ANALYST): ";
        std::string role_choice;
        std::getline(std::cin, role_choice);

        Role role = Role::ANALYST; // 默认角色
        if (role_choice == "1")
            role = Role::DBA;
        else if (role_choice == "2")
            role = Role::DEVELOPER;
        else if (role_choice == "3")
            role = Role::ANALYST;

        if (auth_service_->createUser(username, password, role)) {
            std::cout << "用户创建成功!" << std::endl;
        } else {
            std::cout << "用户创建失败!" << std::endl;
        }
    } else if (choice == "2") {
        std::cout << "请输入要删除的用户名: ";
        std::string username;
        std::getline(std::cin, username);

        if (auth_service_->deleteUser(username)) {
            std::cout << "用户删除成功!" << std::endl;
        } else {
            std::cout << "用户删除失败!" << std::endl;
        }
    } else if (choice == "3") {
        auto users = auth_service_->listUsers();
        std::cout << "\n用户列表:" << std::endl;
        for (const auto& user : users) {
            std::cout << "- " << user << std::endl;
        }
    } else if (choice == "4") {
        return;
    } else {
        std::cout << "无效选择，请重试。" << std::endl;
    }
}

void AuthCLI::processCommand(const std::string& command) {
    std::istringstream iss(command);
    std::string cmd;
    iss >> cmd;

    if (cmd == ".help") {
        showHelp();
    } else if (cmd == ".tables") {
        showTables();
    } else if (cmd == ".create") {
        std::string table_name;
        iss >> table_name;
        if (!table_name.empty()) {
            createTable(table_name);
        } else {
            std::cout << "请指定表名" << std::endl;
        }
    } else if (cmd == ".insert") {
        std::string table_name;
        iss >> table_name;
        if (!table_name.empty()) {
            insertData(table_name);
        } else {
            std::cout << "请指定表名" << std::endl;
        }
    } else if (cmd == ".select") {
        std::string table_name;
        iss >> table_name;
        if (!table_name.empty()) {
            selectData(table_name);
        } else {
            std::cout << "请指定表名" << std::endl;
        }
    } else if (cmd == ".delete") {
        std::string table_name;
        iss >> table_name;
        if (!table_name.empty()) {
            deleteData(table_name);
        } else {
            std::cout << "请指定表名" << std::endl;
        }
    } else if (cmd == ".users") {
        manageUsers();
    } else if (cmd == ".back") {
        return;
    } else {
        std::cout << "未知命令: " << cmd << std::endl;
        std::cout << "输入 .back 返回主菜单" << std::endl;
    }
}

bool AuthCLI::checkPermission(Permission permission) const {
    if (!is_authenticated_)
        return false;
    return auth_service_->hasPermission(permission);
}

void AuthCLI::showPermissionDenied(const std::string& operation) const {
    std::cout << "[权限拒绝] 您没有权限执行: " << operation << std::endl;
    std::cout << "当前角色: " << getCurrentUserRole() << std::endl;
}

} // namespace minidb