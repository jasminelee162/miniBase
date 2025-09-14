#include "AuthCLI.h"
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>

namespace minidb {

AuthCLI::AuthCLI() : is_authenticated_(false), current_user_("") {
    initializeDatabase();
}

void AuthCLI::initializeDatabase() {
    // 初始化存储引擎和目录
    storage_engine_ = std::make_unique<StorageEngine>("data/minidb.bin", 16);
    catalog_ = std::make_unique<Catalog>(storage_engine_.get());
    
    // 尝试从存储加载目录信息
    try {
        catalog_->LoadFromStorage(storage_engine_.get());
        std::cout << "[AuthCLI] 从现有数据库加载完成" << std::endl;
    } catch (...) {
        std::cout << "[AuthCLI] 创建新数据库" << std::endl;
    }
    
    auth_service_ = std::make_unique<AuthService>(storage_engine_.get(), catalog_.get());
    std::cout << "[AuthCLI] 数据库初始化完成" << std::endl;
}

bool AuthCLI::login(const std::string& username, const std::string& password) {
    if (auth_service_->login(username, password)) {
        is_authenticated_ = true;
        current_user_ = username;
        std::cout << "[AuthCLI] 登录成功！欢迎 " << username << std::endl;
        std::cout << "[AuthCLI] 用户角色: " << getCurrentUserRole() << std::endl;
        return true;
    } else {
        std::cout << "[AuthCLI] 登录失败！用户名或密码错误" << std::endl;
        return false;
    }
}

void AuthCLI::logout() {
    if (is_authenticated_) {
        auth_service_->logout();
        is_authenticated_ = false;
        current_user_ = "";
        std::cout << "[AuthCLI] 已退出登录" << std::endl;
    }
}

bool AuthCLI::isAuthenticated() const {
    return is_authenticated_;
}

std::string AuthCLI::getCurrentUser() const {
    return current_user_;
}

std::string AuthCLI::getCurrentUserRole() const {
    if (!is_authenticated_) return "未登录";
    
    Role role = auth_service_->getCurrentUserRole();
    switch (role) {
        case Role::DBA: return "管理员";
        case Role::DEVELOPER: return "开发者";
        case Role::ANALYST: return "分析师";
        default: return "未知";
    }
}

void AuthCLI::run() {
    showWelcomeMessage();
    
    // 登录循环
    while (!is_authenticated_) {
        showLoginPrompt();
        std::string username, password;
        
        std::cout << "用户名: ";
        std::getline(std::cin, username);
        
        if (username.empty()) {
            std::cout << "用户名不能为空！" << std::endl;
            continue;
        }
        
        std::cout << "密码: ";
        std::getline(std::cin, password);
        
        if (password.empty()) {
            std::cout << "密码不能为空！" << std::endl;
            continue;
        }
        
        login(username, password);
    }
    
    // 主菜单循环
    showMainMenu();
    std::string command;
    
    while (true) {
        std::cout << "\nminidb [" << current_user_ << ":" << getCurrentUserRole() << "]> ";
        std::getline(std::cin, command);
        
        if (command.empty()) continue;
        
        if (command == ".exit" || command == "quit") {
            logout();
            break;
        }
        
        processCommand(command);
    }
}

void AuthCLI::showWelcomeMessage() {
    std::cout << "===============================================" << std::endl;
    std::cout << "          欢迎使用 MiniDB 数据库系统" << std::endl;
    std::cout << "===============================================" << std::endl;
    std::cout << "请先登录以使用数据库功能" << std::endl;
    std::cout << "默认管理员账户: root / root" << std::endl;
    std::cout << "===============================================" << std::endl;
}

void AuthCLI::showLoginPrompt() {
    std::cout << "\n--- 用户登录 ---" << std::endl;
}

void AuthCLI::showMainMenu() {
    std::cout << "\n===============================================" << std::endl;
    std::cout << "           MiniDB 主菜单" << std::endl;
    std::cout << "===============================================" << std::endl;
    showHelp();
}

void AuthCLI::showHelp() {
    std::cout << "\n可用命令:" << std::endl;
    std::cout << "  .help           - 显示帮助信息" << std::endl;
    std::cout << "  .info           - 显示当前用户信息" << std::endl;
    std::cout << "  .tables         - 显示所有表" << std::endl;
    std::cout << "  .create <表名>   - 创建表" << std::endl;
    std::cout << "  .insert <表名>   - 插入数据" << std::endl;
    std::cout << "  .select <表名>   - 查询数据" << std::endl;
    std::cout << "  .delete <表名>   - 删除数据" << std::endl;
    std::cout << "  .users           - 管理用户 (仅管理员)" << std::endl;
    std::cout << "  .exit            - 退出系统" << std::endl;
    std::cout << "  SQL语句          - 执行SQL命令" << std::endl;
}

void AuthCLI::processCommand(const std::string& command) {
    std::istringstream iss(command);
    std::string cmd;
    iss >> cmd;
    
    if (cmd == ".help") {
        showHelp();
    }
    else if (cmd == ".info") {
        showUserInfo();
    }
    else if (cmd == ".tables") {
        showTables();
    }
    else if (cmd == ".create") {
        std::string table_name;
        iss >> table_name;
        if (table_name.empty()) {
            std::cout << "用法: .create <表名>" << std::endl;
        } else {
            createTable(table_name);
        }
    }
    else if (cmd == ".insert") {
        std::string table_name;
        iss >> table_name;
        if (table_name.empty()) {
            std::cout << "用法: .insert <表名>" << std::endl;
        } else {
            insertData(table_name);
        }
    }
    else if (cmd == ".select") {
        std::string table_name;
        iss >> table_name;
        if (table_name.empty()) {
            std::cout << "用法: .select <表名>" << std::endl;
        } else {
            selectData(table_name);
        }
    }
    else if (cmd == ".delete") {
        std::string table_name;
        iss >> table_name;
        if (table_name.empty()) {
            std::cout << "用法: .delete <表名>" << std::endl;
        } else {
            deleteData(table_name);
        }
    }
    else if (cmd == ".users") {
        manageUsers();
    }
    else {
        // 尝试作为SQL语句执行
        std::cout << "执行SQL: " << command << std::endl;
        std::cout << "注意: SQL执行功能正在开发中..." << std::endl;
    }
}

void AuthCLI::showUserInfo() {
    std::cout << "\n--- 当前用户信息 ---" << std::endl;
    std::cout << "用户名: " << current_user_ << std::endl;
    std::cout << "角色: " << getCurrentUserRole() << std::endl;
    
    // 显示权限
    std::cout << "权限: ";
    auto permissions = auth_service_->getCurrentUserPermissions();
    for (size_t i = 0; i < permissions.size(); ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << auth_service_->permissionToString(permissions[i]);
    }
    std::cout << std::endl;
}

void AuthCLI::showTables() {
    if (!checkPermission(Permission::SELECT)) {
        showPermissionDenied("查看表");
        return;
    }
    
    std::cout << "\n--- 数据库表列表 ---" << std::endl;
    // 这里应该从catalog获取表列表，暂时显示用户表
    std::cout << "__users__ (用户表)" << std::endl;
    std::cout << "注意: 表列表功能正在完善中..." << std::endl;
}

void AuthCLI::createTable(const std::string& table_name) {
    if (!checkPermission(Permission::CREATE_TABLE)) {
        showPermissionDenied("创建表");
        return;
    }
    
    std::cout << "创建表: " << table_name << std::endl;
    std::cout << "注意: 表创建功能正在开发中..." << std::endl;
}

void AuthCLI::insertData(const std::string& table_name) {
    if (!checkPermission(Permission::INSERT)) {
        showPermissionDenied("插入数据");
        return;
    }
    
    std::cout << "向表 " << table_name << " 插入数据" << std::endl;
    std::cout << "注意: 数据插入功能正在开发中..." << std::endl;
}

void AuthCLI::selectData(const std::string& table_name) {
    if (!checkPermission(Permission::SELECT)) {
        showPermissionDenied("查询数据");
        return;
    }
    
    std::cout << "查询表 " << table_name << " 的数据" << std::endl;
    std::cout << "注意: 数据查询功能正在开发中..." << std::endl;
}

void AuthCLI::deleteData(const std::string& table_name) {
    if (!checkPermission(Permission::DELETE)) {
        showPermissionDenied("删除数据");
        return;
    }
    
    std::cout << "删除表 " << table_name << " 的数据" << std::endl;
    std::cout << "注意: 数据删除功能正在开发中..." << std::endl;
}

void AuthCLI::manageUsers() {
    if (!checkPermission(Permission::CREATE_USER)) {
        showPermissionDenied("用户管理");
        return;
    }
    
    std::cout << "\n--- 用户管理 ---" << std::endl;
    std::cout << "当前用户列表:" << std::endl;
    
    auto users = auth_service_->listUsers();
    for (const auto& user : users) {
        std::cout << "  - " << user << std::endl;
    }
    
    std::cout << "\n用户管理命令:" << std::endl;
    std::cout << "  .create_user <用户名> <密码> <角色>  - 创建用户" << std::endl;
    std::cout << "  .delete_user <用户名>                - 删除用户" << std::endl;
    std::cout << "  .list_users                          - 列出所有用户" << std::endl;
    std::cout << "  .back                                - 返回主菜单" << std::endl;
    
    std::string command;
    while (true) {
        std::cout << "\n用户管理> ";
        std::getline(std::cin, command);
        
        if (command.empty()) continue;
        
        std::istringstream iss(command);
        std::string cmd;
        iss >> cmd;
        
        if (cmd == ".back" || cmd == "back") {
            break;
        }
        else if (cmd == ".create_user") {
            std::string username, password, role_str;
            iss >> username >> password >> role_str;
            
            if (username.empty() || password.empty() || role_str.empty()) {
                std::cout << "用法: .create_user <用户名> <密码> <角色>" << std::endl;
                std::cout << "角色: dba, developer, analyst" << std::endl;
                continue;
            }
            
            Role role;
            if (role_str == "dba") {
                role = Role::DBA;
            } else if (role_str == "developer") {
                role = Role::DEVELOPER;
            } else if (role_str == "analyst") {
                role = Role::ANALYST;
            } else {
                std::cout << "无效角色！请使用: dba, developer, analyst" << std::endl;
                continue;
            }
            
            if (auth_service_->createUser(username, password, role)) {
                std::cout << "用户 " << username << " 创建成功！" << std::endl;
            } else {
                std::cout << "用户 " << username << " 创建失败！" << std::endl;
            }
        }
        else if (cmd == ".delete_user") {
            std::string username;
            iss >> username;
            
            if (username.empty()) {
                std::cout << "用法: .delete_user <用户名>" << std::endl;
                continue;
            }
            
            if (username == "root") {
                std::cout << "不能删除root用户！" << std::endl;
                continue;
            }
            
            if (auth_service_->deleteUser(username)) {
                std::cout << "用户 " << username << " 删除成功！" << std::endl;
            } else {
                std::cout << "用户 " << username << " 删除失败！" << std::endl;
            }
        }
        else if (cmd == ".list_users") {
            std::cout << "\n当前用户列表:" << std::endl;
            auto users = auth_service_->listUsers();
            for (const auto& user : users) {
                std::cout << "  - " << user << std::endl;
            }
        }
        else {
            std::cout << "未知命令: " << cmd << std::endl;
            std::cout << "输入 .back 返回主菜单" << std::endl;
        }
    }
}

bool AuthCLI::checkPermission(Permission permission) const {
    if (!is_authenticated_) return false;
    return auth_service_->hasPermission(permission);
}

void AuthCLI::showPermissionDenied(const std::string& operation) const {
    std::cout << "[权限拒绝] 您没有权限执行: " << operation << std::endl;
    std::cout << "当前角色: " << getCurrentUserRole() << std::endl;
    std::cout << "请联系管理员获取相应权限" << std::endl;
}

} // namespace minidb
