#include "user_management.h"

#include "../auth/auth_service.h"

#include <iostream>
#include <sstream>

namespace minidb {
namespace cli {

std::string role_to_cn(const std::string &roleStr)
{
    if (roleStr == "DBA") return "管理员";
    if (roleStr == "DEVELOPER") return "开发者";
    if (roleStr == "ANALYST") return "分析师";
    return "未知";
}

void print_user_info(AuthService *auth)
{
    if (!auth || !auth->isLoggedIn())
    {
        std::cout << "\n--- 当前用户信息 ---\n未登录" << std::endl;
        return;
    }
    std::cout << "\n--- 当前用户信息 ---" << std::endl;
    std::cout << "用户名: " << auth->getCurrentUser() << std::endl;
    auto roleStr = auth->getCurrentUserRoleString();
    std::cout << "角色: " << role_to_cn(roleStr) << std::endl;
    std::cout << "权限: ";
    auto perms = auth->getCurrentUserPermissions();
    for (size_t i = 0; i < perms.size(); ++i)
    {
        if (i) std::cout << ", ";
        std::cout << auth->permissionToString(perms[i]);
    }
    std::cout << std::endl;
}

void manage_users(AuthService *auth)
{
    if (!auth || !auth->isLoggedIn())
    {
        std::cout << "[权限拒绝] 请先登录" << std::endl;
        return;
    }
    if (!auth->isDBA())
    {
        std::cout << "[权限拒绝] 您没有权限执行: 用户管理" << std::endl;
        return;
    }

    std::cout << "\n--- 用户管理 ---" << std::endl;
    std::cout << "当前用户列表:" << std::endl;
    for (const auto &u : auth->listUsers())
        std::cout << "  - " << u << std::endl;

    std::cout << "\n用户管理命令:" << std::endl;
    std::cout << "  .create_user <用户名> <密码> <角色>  - 创建用户" << std::endl;
    std::cout << "  .delete_user <用户名>                - 删除用户" << std::endl;
    std::cout << "  .list_users                          - 列出所有用户" << std::endl;
    std::cout << "  .back                                - 返回主菜单" << std::endl;

    std::string command;
    while (true)
    {
        std::cout << "\n用户管理> ";
        if (!std::getline(std::cin, command)) break;
        if (command.empty()) continue;

        std::istringstream iss(command);
        std::string cmd;
        iss >> cmd;

        if (cmd == ".back" || cmd == "back")
        {
            break;
        }
        else if (cmd == ".create_user")
        {
            std::string username, password, role_str;
            iss >> username >> password >> role_str;
            if (username.empty() || password.empty() || role_str.empty())
            {
                std::cout << "用法: .create_user <用户名> <密码> <角色>" << std::endl;
                std::cout << "角色: dba, developer, analyst" << std::endl;
                continue;
            }
            Role role;
            if (role_str == "dba") role = Role::DBA;
            else if (role_str == "developer") role = Role::DEVELOPER;
            else if (role_str == "analyst") role = Role::ANALYST;
            else {
                std::cout << "无效角色！请使用: dba, developer, analyst" << std::endl;
                continue;
            }
            if (auth->createUser(username, password, role))
                std::cout << "用户 " << username << " 创建成功！" << std::endl;
            else
                std::cout << "用户 " << username << " 创建失败！" << std::endl;
        }
        else if (cmd == ".delete_user")
        {
            std::string username; iss >> username;
            if (username.empty()) { std::cout << "用法: .delete_user <用户名>" << std::endl; continue; }
            if (username == "root") { std::cout << "不能删除root用户！" << std::endl; continue; }
            if (auth->deleteUser(username))
                std::cout << "用户 " << username << " 删除成功！" << std::endl;
            else
                std::cout << "用户 " << username << " 删除失败！" << std::endl;
        }
        else if (cmd == ".list_users")
        {
            std::cout << "\n当前用户列表:" << std::endl;
            for (const auto &u : auth->listUsers())
                std::cout << "  - " << u << std::endl;
        }
        else
        {
            std::cout << "未知命令: " << cmd << std::endl;
            std::cout << "输入 .back 返回主菜单" << std::endl;
        }
    }
}

} // namespace cli
} // namespace minidb


