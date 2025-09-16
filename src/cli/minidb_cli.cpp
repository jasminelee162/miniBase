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
#include <ctime>
#include <cstdio>
#include <filesystem>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include "minidb/cli_helpers.h"
#include "minidb/user_management.h"
#include "minidb/execution_pipeline.h"
#include "minidb/command_handlers.h"
#include "minidb/input_accumulator.h"


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
            minidb::cli::print_help();
            minidb::cli::print_help();
            return 0;
        }
        else
        {
            std::cerr << "Unknown argument: " << arg << std::endl;
            minidb::cli::print_help();
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
        exec = std::make_unique<minidb::Executor>(catalog, permissionChecker.get());
        exec->SetAuthService(authService.get());
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
                std::cout << "[Auth] 用户角色: " << minidb::cli::role_to_cn(authService->getCurrentUserRoleString()) << std::endl;
                break;
            }
            else
            {
                std::cout << "[Auth] 登录失败！用户名或密码错误" << std::endl;
            }
        }
    }

    minidb::cli::InputAccumulator accumulator;

    while (true)
    {
        std::string promptUser;
        if (doExec && authService && authService->isLoggedIn())
        {
            promptUser = minidb::cli::make_prompt(authService.get());
            std::cout << "[minidb " << promptUser << ">> " << std::flush;
        }
        else
        {
            std::cout << ">> " << std::flush;
        }
        std::string line;
        if (!std::getline(std::cin, line))
        {
            // 清理资源后安全退出
            if (exec)
                exec.reset();
            if (permissionChecker)
                permissionChecker.reset();
            if (authService)
            {
                if (authService->isLoggedIn()) authService->logout();
                authService.reset();
            }
            if (catalog)
                catalog.reset();
            if (se)
                se.reset();
            break;
        }
        if (line == ".exit")
        {
            // 清理资源后安全退出
            if (exec)
                exec.reset();
            if (permissionChecker)
                permissionChecker.reset();
            if (authService)
            {
                if (authService->isLoggedIn()) authService->logout();
                authService.reset();
            }
            if (catalog)
                catalog.reset();
            if (se)
                se.reset();
            break;
        }
        if (minidb::cli::handle_help(line)) continue;
        if (line == ".exit")
        {
            if (exec) exec.reset();
            if (permissionChecker) permissionChecker.reset();
            if (authService) { if (authService->isLoggedIn()) authService->logout(); authService.reset(); }
            if (catalog) catalog.reset();
            if (se) se.reset();
            break;
        }
        if (line == ".login" && !doExec) { std::cout << "仅执行模式可登录" << std::endl; continue; }
        if (line == ".logout" && !doExec) { std::cout << "仅执行模式可登出" << std::endl; continue; }
        if (minidb::cli::handle_login(line, authService.get())) continue;
        if (minidb::cli::handle_logout(line, authService.get())) continue;
        if (minidb::cli::handle_info(line, authService.get())) continue;
        if (minidb::cli::handle_users(line, authService.get())) continue;
        
        // 处理导入导出命令
        if (line.rfind(".dump ", 0) == 0)
        {
            if (!minidb::cli::require_exec_mode(doExec, "Error: Export requires execution mode. Use --exec flag.")) continue;
            if (!minidb::cli::require_logged_in(authService.get())) continue;
            if (minidb::cli::handle_dump(line, catalog.get(), se.get())) continue;
        }

        if (line.rfind(".export ", 0) == 0)
        {
            if (!minidb::cli::require_exec_mode(doExec, "Error: Export requires execution mode. Use --exec flag.")) continue;
            if (!minidb::cli::require_logged_in(authService.get())) continue;
            if (minidb::cli::handle_export_cmd(line, catalog.get(), se.get())) continue;
        }
        
        if (line.rfind(".import ", 0) == 0)
        {
            if (!minidb::cli::require_exec_mode(doExec, "Error: Import requires execution mode. Use --exec flag.")) continue;
            if (!minidb::cli::require_logged_in(authService.get())) continue;
            if (minidb::cli::handle_import_cmd(line, exec.get(), catalog.get())) continue;
        }
        
        
        if (line.empty())
            continue;

        if (doExec && (!authService || !authService->isLoggedIn()))
        {
            minidb::cli::require_logged_in(authService.get());
            continue;
        }

        accumulator.append_line(line);
        if (!accumulator.ready())
                continue;

        std::string buffer = accumulator.take();
            // 首关键词自动纠错（距离<=1）
        std::string toParse = minidb::cliutil::autocorrect_leading_keyword(buffer);
        minidb::cli::execute_sql_pipeline(
            toParse,
            catalog.get(),
            exec.get(),
            se.get(),
            outputJsonOnly
        );
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
