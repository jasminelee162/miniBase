#include "command_handlers.h"

#include "cli_helpers.h"
#include "user_management.h"

#include "../tools/sql_dump/sql_dumper.h"
#include "../tools/sql_import/sql_importer.h"
#include "../auth/auth_service.h"

#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <sstream>

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

bool handle_debug_fullscan(const std::string &line, Catalog *catalog, StorageEngine *se)
{
    if (line.rfind(".debug_fullscan ", 0) == 0)
    {
        std::string table = trim_copy(line.substr(16));
        if (table.empty()) { std::cout << "用法: .debug_fullscan <table>" << std::endl; return true; }
        if (!catalog->HasTable(table)) { std::cout << "表不存在: " << table << std::endl; return true; }
        const auto schema = catalog->GetTable(table);
        size_t total = 0;
        size_t num_pages = se->GetNumPages();
        for (page_id_t pid = 1; pid < static_cast<page_id_t>(num_pages); ++pid)
        {
            Page *p = se->GetDataPage(pid);
            if (!p) continue;
            auto records = se->GetPageRecords(p);
            for (auto &rec : records)
            {
                auto row = Row::Deserialize(reinterpret_cast<const unsigned char*>(rec.first), rec.second, schema);
                if (!row.columns.empty()) total++;
            }
            se->PutPage(pid, false);
        }
        std::cout << "[Debug] fullscan " << table << ": " << total << " rows" << std::endl;
        return true;
    }
    return false;
}

bool handle_debug_set_firstpage(const std::string &line, Catalog *catalog)
{
    if (line.rfind(".debug_set_firstpage ", 0) == 0)
    {
        std::istringstream iss(line.substr(21));
        std::string table; page_id_t pid;
        if (!(iss >> table >> pid)) { std::cout << "用法: .debug_set_firstpage <table> <page_id>" << std::endl; return true; }
        if (!catalog->HasTable(table)) { std::cout << "表不存在: " << table << std::endl; return true; }
        if (catalog->UpdateTableFirstPageId(table, pid)) {
            std::cout << "已设置首页: " << table << " => " << pid << std::endl;
        } else {
            std::cout << "设置失败: " << table << std::endl;
        }
        return true;
    }
    return false;
}

bool handle_debug_guess_firstpage(const std::string &line, Catalog *catalog, StorageEngine *se)
{
    if (line.rfind(".debug_guess_firstpage ", 0) == 0)
    {
        std::string table = trim_copy(line.substr(23));
        if (table.empty()) { std::cout << "用法: .debug_guess_firstpage <table>" << std::endl; return true; }
        if (!catalog->HasTable(table)) { std::cout << "表不存在: " << table << std::endl; return true; }
        size_t num_pages = se->GetNumPages();
        // 建立 to->from 反向映射
        std::unordered_map<page_id_t, page_id_t> to_from;
        std::unordered_set<page_id_t> candidates;
        for (page_id_t pid = 1; pid < static_cast<page_id_t>(num_pages); ++pid)
        {
            Page *p = se->GetDataPage(pid);
            if (!p) continue;
            page_id_t nxt = p->GetNextPageId();
            if (nxt != INVALID_PAGE_ID) to_from[nxt] = pid;
            candidates.insert(pid);
            se->PutPage(pid, false);
        }
        // 头结点满足: 不作为任何页面的 next
        std::vector<page_id_t> heads;
        for (auto pid : candidates) if (!to_from.count(pid)) heads.push_back(pid);
        if (heads.empty()) { std::cout << "未能猜测首页（可能链断裂/单页表）" << std::endl; return true; }
        std::cout << "可能的首页候选: ";
        for (size_t i = 0; i < heads.size(); ++i) { if (i) std::cout << ", "; std::cout << heads[i]; }
        std::cout << std::endl;
        std::cout << "使用: .debug_set_firstpage " << table << " <pid> 进行设置" << std::endl;
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
            log_info(std::string("login success: ") + username + ", role=" + auth->getCurrentUserRoleString());
            std::cout << "登录成功" << std::endl;
            std::cout << "角色: " << role_to_cn(auth->getCurrentUserRoleString()) << std::endl;
        }
        else
        {
            log_warn(std::string("login failed for user: ") + username);
            std::cout << "登录失败：用户名或密码错误" << std::endl;
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
        log_info("logout");
        std::cout << "已退出登录" << std::endl;
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
            log_warn("dump: missing output filename");
            std::cerr << "Error: Please specify output filename for dump command." << std::endl;
            return true;
        }
        SQLDumper dumper(catalog, se);
        std::string out = resolve_export_output_path(filename);
        if (out.empty()) {
            log_error("dump: invalid output path");
            std::cerr << "Error: Invalid output path." << std::endl;
            return true;
        }
        if (dumper.DumpToFile(out, DumpOption::StructureAndData)) {
            log_info(std::string("dump succeeded: ") + out);
            std::cout << "导出成功: " << out << std::endl;
        } else {
            log_error(std::string("dump failed: ") + out);
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
            log_warn("export: missing output path");
            std::cerr << "Error: Please specify output path (directory or file) for export." << std::endl;
            return true;
        }
        SQLDumper dumper(catalog, se);
        std::string out = resolve_export_output_path(pathArg);
        if (out.empty()) {
            log_error("export: invalid output path");
            std::cerr << "Error: Invalid output path." << std::endl;
            return true;
        }
        if (dumper.DumpToFile(out, DumpOption::StructureAndData)) {
            log_info(std::string("export succeeded: ") + out);
            std::cout << "导出成功: " << out << std::endl;
        } else {
            log_error(std::string("export failed: ") + out);
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
            log_warn("import: missing input filename");
            std::cerr << "Error: Please specify input filename for import command." << std::endl;
            return true;
        }
        SQLImporter importer(exec, catalog);
        if (importer.ImportSQLFile(filename)) {
            log_info(std::string("import succeeded from ") + filename);
            std::cout << "导入成功: " << filename << std::endl;
        } else {
            log_error(std::string("import failed from ") + filename);
            std::cerr << "Error: Failed to import database from " << filename << std::endl;
        }
        return true;
    }
    return false;
}

} // namespace cli
} // namespace minidb


