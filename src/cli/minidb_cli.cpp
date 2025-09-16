#include "../sql_compiler/lexer/lexer.h"
#include "../sql_compiler/parser/parser.h"
#include "../sql_compiler/parser/ast_json_serializer.h"
#include "../sql_compiler/semantic/semantic_analyzer.h"
#include "../frontend/translator/json_to_plan.h"
#include "../storage/storage_engine.h"
#include "../engine/executor/executor.h"
#include "../catalog/catalog.h"
#include "../util/table_utils.h"
#include <iostream>
#include <string>
#include <memory>
#include <fstream>
#include <algorithm>
#include <vector>
#include <cctype>
#include "../util/sql_input_utils.h"
#include "../tools/sql_dump/sql_dumper.h"
#include "../tools/sql_import/sql_importer.h"
#include "../auth/permission_checker.h"
#include "../auth/auth_service.h"
#include <sstream>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

static void print_help()
{
    std::cout << "MiniDB CLI\n"
              << "Usage: minidb_cli [--exec|--json] [--db <path>]\n"
              << "Commands:\n"
              << "  .help           Show this help\n"
              << "  .login          Login user (or re-login)\n"
              << "  .logout         Logout current user\n"
              << "  .info           Show current user info\n"
              << "  .users          Manage users (DBA only)\n"
              << "  .exit           Quit\n"
              << "  .dump <file>    Export database to SQL file\n"
              << "  .import <file>  Import SQL file to database\n"
              << "Enter SQL terminated by ';' to run.\n";
}

using minidb::cliutil::autocorrect_leading_keyword;
using minidb::cliutil::can_terminate_without_semicolon;

static std::string role_to_cn(const std::string &roleStr)
{
    if (roleStr == "DBA") return "管理员";
    if (roleStr == "DEVELOPER") return "开发者";
    if (roleStr == "ANALYST") return "分析师";
    return "未知";
}

static void print_user_info(minidb::AuthService *auth)
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

static void manage_users(minidb::AuthService *auth)
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
            minidb::Role role;
            if (role_str == "dba") role = minidb::Role::DBA;
            else if (role_str == "developer") role = minidb::Role::DEVELOPER;
            else if (role_str == "analyst") role = minidb::Role::ANALYST;
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

static int count_substring(const std::string &s, const std::string &pat)
{
    if (pat.empty()) return 0;
    int cnt = 0;
    size_t pos = 0;
    while ((pos = s.find(pat, pos)) != std::string::npos)
    {
        cnt++;
        pos += pat.size();
    }
    return cnt;
}

int main(int argc, char **argv)
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    bool doExec = true;
    bool outputJsonOnly = false;
    std::string dbFile = "data/mini.db";

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--exec")
        {
            doExec = true;
            outputJsonOnly = false;
        }
        else if (arg == "--json")
        {
            outputJsonOnly = true;
            doExec = false;
        }
        else if (arg == "--db" && i + 1 < argc)
        {
            dbFile = argv[++i];
        }
        else if (arg == "-h" || arg == "--help")
        {
            print_help();
            return 0;
        }
        else
        {
            std::cerr << "Unknown argument: " << arg << std::endl;
            print_help();
            return 1;
        }
    }

    std::shared_ptr<minidb::StorageEngine> se;
    std::shared_ptr<minidb::Catalog> catalog;
    std::unique_ptr<minidb::Executor> exec;
    std::unique_ptr<minidb::PermissionChecker> permissionChecker;
    std::unique_ptr<minidb::AuthService> authService;
    if (doExec)
    {
        se = std::make_shared<minidb::StorageEngine>(dbFile, 16);
        catalog = std::make_shared<minidb::Catalog>(se.get());
        authService = std::make_unique<minidb::AuthService>(se.get(), catalog.get());
        permissionChecker = std::make_unique<minidb::PermissionChecker>(authService.get());
        exec = std::make_unique<minidb::Executor>(catalog.get(), permissionChecker.get());
        exec->SetStorageEngine(se);
    }

    std::cout << "MiniDB CLI ready. Type .help for help.\n";
    if (doExec)
    {
        std::cout << "请先登录以使用数据库功能 (默认 root / root)" << std::endl;
        while (true)
        {
            if (authService->isLoggedIn()) break;
            std::string username, password;
            std::cout << "用户名: ";
            if (!std::getline(std::cin, username)) break;
            if (username.empty()) { std::cout << "用户名不能为空！" << std::endl; continue; }
            std::cout << "密码: ";
            if (!std::getline(std::cin, password)) break;
            if (password.empty()) { std::cout << "密码不能为空！" << std::endl; continue; }
            if (authService->login(username, password))
            {
                std::cout << "[Auth] 登录成功！欢迎 " << username << std::endl;
                std::cout << "[Auth] 用户角色: " << role_to_cn(authService->getCurrentUserRoleString()) << std::endl;
                break;
            }
            else
            {
                std::cout << "[Auth] 登录失败！用户名或密码错误" << std::endl;
            }
        }
    }

    std::string buffer;
    buffer.reserve(1024);

    while (true)
    {
        std::string promptUser;
        if (doExec && authService && authService->isLoggedIn())
        {
            promptUser = authService->getCurrentUser() + ":" + role_to_cn(authService->getCurrentUserRoleString()) + "] ";
            std::cout << "[minidb " << promptUser << ">> " << std::flush;
        }
        else
        {
            std::cout << ">> " << std::flush;
        }
        std::string line;
        if (!std::getline(std::cin, line))
            break;
        if (line == ".exit")
            break;
        if (line == ".help")
        {
            print_help();
            continue;
        }

        if (line == ".login")
        {
            if (!doExec) { std::cout << "仅执行模式可登录" << std::endl; continue; }
            std::string username, password;
            std::cout << "用户名: ";
            if (!std::getline(std::cin, username)) continue;
            std::cout << "密码: ";
            if (!std::getline(std::cin, password)) continue;
            if (authService->login(username, password))
            {
                std::cout << "[Auth] 登录成功！欢迎 " << username << std::endl;
                std::cout << "[Auth] 用户角色: " << role_to_cn(authService->getCurrentUserRoleString()) << std::endl;
            }
            else
            {
                std::cout << "[Auth] 登录失败！用户名或密码错误" << std::endl;
            }
            continue;
        }
        if (line == ".logout")
        {
            if (!doExec) { std::cout << "仅执行模式可登出" << std::endl; continue; }
            authService->logout();
            std::cout << "[Auth] 已退出登录" << std::endl;
            continue;
        }
        if (line == ".info")
        {
            print_user_info(authService.get());
            continue;
        }
        if (line == ".users")
        {
            manage_users(authService.get());
            continue;
        }
        
        // 处理导入导出命令
        if (line.substr(0, 6) == ".dump ")
        {
            if (!doExec) {
                std::cerr << "Error: Export requires execution mode. Use --exec flag." << std::endl;
                continue;
            }
            if (!authService->isLoggedIn()) { std::cout << "[权限拒绝] 请先登录" << std::endl; continue; }
            std::string filename = line.substr(6);
            if (filename.empty()) {
                std::cerr << "Error: Please specify output filename for dump command." << std::endl;
                continue;
            }
            
            minidb::SQLDumper dumper(catalog.get(), se.get());
            if (dumper.DumpToFile(filename, minidb::DumpOption::StructureAndData)) {
                std::cout << "Database exported to: " << filename << std::endl;
            } else {
                std::cerr << "Error: Failed to export database to " << filename << std::endl;
            }
            continue;
        }
        
        if (line.substr(0, 8) == ".import ")
        {
            if (!doExec) {
                std::cerr << "Error: Import requires execution mode. Use --exec flag." << std::endl;
                continue;
            }
            if (!authService->isLoggedIn()) { std::cout << "[权限拒绝] 请先登录" << std::endl; continue; }
            std::string filename = line.substr(8);
            if (filename.empty()) {
                std::cerr << "Error: Please specify input filename for import command." << std::endl;
                continue;
            }
            
            minidb::SQLImporter importer(exec.get(), catalog.get());
            if (importer.ImportSQLFile(filename)) {
                std::cout << "Database imported from: " << filename << std::endl;
            } else {
                std::cerr << "Error: Failed to import database from " << filename << std::endl;
            }
            continue;
        }
        
        
        if (line.empty())
            continue;

        if (doExec && (!authService || !authService->isLoggedIn()))
        {
            std::cout << "[权限拒绝] 请先登录 (.login)" << std::endl;
            continue;
        }

        buffer += line;
        // 若存在未闭合的 BEGIN/END 块，则继续读入，直至匹配的 END; 出现
        int beginCount = count_substring(buffer, "BEGIN");
        int endCount = count_substring(buffer, "END");
        bool hasUnclosedBlock = beginCount > endCount;

        if (buffer.find(';') == std::string::npos || hasUnclosedBlock)
        {
            if (can_terminate_without_semicolon(buffer, line))
            {
                // 允许在空行且括号配平时，无分号结束
            }
            else
            {
                buffer += '\n';
                continue;
            }
        }

        try
        {
            // 首关键词自动纠错（距离<=1）
            std::string toParse = autocorrect_leading_keyword(buffer);
            Lexer l(toParse);
            auto tokens = l.tokenize();
            Parser p(tokens);
            auto stmt = p.parse();

            // 语义检查
            try
            {
                SemanticAnalyzer sem;
                sem.setCatalog(catalog.get());
                sem.analyze(stmt.get());
            }
            catch (const std::exception &e)
            {
                std::cerr << "[Semantic][ERROR] " << e.what() << std::endl;
                // 语义错误为致命错误：跳过执行阶段
                buffer.clear();
                continue;
            }

            auto j = ASTJson::toJson(stmt.get());
            std::cout << "astjson: " << j.dump(2) << std::endl;

            if (outputJsonOnly)
            {
                std::cout << "只有astjson: " << j.dump(2) << std::endl;
            }
            else if (doExec)
            {
                auto plan = JsonToPlan::translate(j);

                // 执行 Plan
                auto results = exec->execute(plan.get());
                std::cout << "[OK] executed." << std::endl;
                TablePrinter::printResults(results, " ");

                // ✅ 强制刷新 Catalog & StorageEngine，保证持久化
                catalog->SaveToStorage();
                // se->flush();
            }
        }
        catch (const ParseError &e)
        {
            std::cerr << "[Parser][ERROR] [minidb_cli] " << e.what()
                      << " at (" << e.getLine() << "," << e.getColumn() << ")" << std::endl;
        }
        catch (const std::exception &e)
        {
            std::cerr << "[ERROR] " << e.what() << std::endl;
        }

        buffer.clear();
    }

    std::cout << "Bye." << std::endl;

    // 显式清理资源，防止后台线程/句柄阻塞退出
    if (doExec)
    {
        if (authService)
        {
            if (authService->isLoggedIn()) authService->logout();
        }
        exec.reset();
        permissionChecker.reset();
        authService.reset();
        catalog.reset();
        se.reset();
    }

    return 0;
}
