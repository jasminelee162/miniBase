#include "command_handlers.h"

#include "cli_helpers.h"
#include "user_management.h"

#include "../tools/sql_dump/sql_dumper.h"
#include "../tools/sql_import/sql_importer.h"
#include "../auth/auth_service.h"

#include <iostream>

namespace minidb {
namespace cli {

bool handle_help(const std::string &line)
{
    if (line == ".help")
    {
        print_help();
        return true;
    }
    return false;
}

bool handle_login(const std::string &line, AuthService *auth)
{
    if (line == ".login")
    {
        std::string username, password;
        std::cout << "用户名: ";
        if (!std::getline(std::cin, username)) return true;
        std::cout << "密码: ";
        if (!std::getline(std::cin, password)) return true;
        if (auth->login(username, password))
        {
            std::cout << "[Auth] 登录成功！欢迎 " << username << std::endl;
            std::cout << "[Auth] 用户角色: " << role_to_cn(auth->getCurrentUserRoleString()) << std::endl;
        }
        else
        {
            std::cout << "[Auth] 登录失败！用户名或密码错误" << std::endl;
        }
        return true;
    }
    return false;
}

bool handle_logout(const std::string &line, AuthService *auth)
{
    if (line == ".logout")
    {
        auth->logout();
        std::cout << "[Auth] 已退出登录" << std::endl;
        return true;
    }
    return false;
}

bool handle_info(const std::string &line, AuthService *auth)
{
    if (line == ".info")
    {
        print_user_info(auth);
        return true;
    }
    return false;
}

bool handle_users(const std::string &line, AuthService *auth)
{
    if (line == ".users")
    {
        manage_users(auth);
        return true;
    }
    return false;
}

bool handle_dump(const std::string &line, Catalog *catalog, StorageEngine *se)
{
    if (line.rfind(".dump ", 0) == 0)
    {
        std::string filename = line.substr(6);
        if (filename.empty()) {
            std::cerr << "Error: Please specify output filename for dump command." << std::endl;
            return true;
        }
        SQLDumper dumper(catalog, se);
        std::string out = resolve_export_output_path(filename);
        if (out.empty()) {
            std::cerr << "Error: Invalid output path." << std::endl;
            return true;
        }
        if (dumper.DumpToFile(out, DumpOption::StructureAndData)) {
            std::cout << "Database exported to: " << out << std::endl;
        } else {
            std::cerr << "Error: Failed to export database to " << out << std::endl;
        }
        return true;
    }
    return false;
}

bool handle_export_cmd(const std::string &line, Catalog *catalog, StorageEngine *se)
{
    if (line.rfind(".export ", 0) == 0)
    {
        std::string pathArg = line.substr(8);
        pathArg = trim_copy(pathArg);
        if (pathArg.empty()) {
            std::cerr << "Error: Please specify output path (directory or file) for export." << std::endl;
            return true;
        }
        SQLDumper dumper(catalog, se);
        std::string out = resolve_export_output_path(pathArg);
        if (out.empty()) {
            std::cerr << "Error: Invalid output path." << std::endl;
            return true;
        }
        if (dumper.DumpToFile(out, DumpOption::StructureAndData)) {
            std::cout << "Database exported to: " << out << std::endl;
        } else {
            std::cerr << "Error: Failed to export database to " << out << std::endl;
        }
        return true;
    }
    return false;
}

bool handle_import_cmd(const std::string &line, Executor *exec, Catalog *catalog)
{
    if (line.rfind(".import ", 0) == 0)
    {
        std::string filename = line.substr(8);
        if (filename.empty()) {
            std::cerr << "Error: Please specify input filename for import command." << std::endl;
            return true;
        }
        SQLImporter importer(exec, catalog);
        if (importer.ImportSQLFile(filename)) {
            std::cout << "Database imported from: " << filename << std::endl;
        } else {
            std::cerr << "Error: Failed to import database from " << filename << std::endl;
        }
        return true;
    }
    return false;
}

} // namespace cli
} // namespace minidb


