#include "cli_helpers.h"

#include <iostream>
#include <filesystem>
#include <sstream>
#include <ctime>
#include <cstdio>
#include <memory>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include "../auth/auth_service.h"
#include "user_management.h"

#ifdef _WIN32
#ifdef ERROR
#undef ERROR
#endif
#ifdef DEBUG
#undef DEBUG
#endif
#ifdef INFO
#undef INFO
#endif
#ifdef WARN
#undef WARN
#endif
#endif

namespace minidb {
namespace cli {

static std::unique_ptr<Logger> g_cli_logger;

void init_cli_logger(const std::string &filepath)
{
    g_cli_logger = std::make_unique<Logger>(filepath);
}

Logger* cli_logger()
{
    return g_cli_logger.get();
}

void set_cli_log_level(Logger::Level level)
{
    if (g_cli_logger) g_cli_logger->setLevel(level);
}

void log_debug(const std::string &msg)
{
    if (g_cli_logger) g_cli_logger->log(static_cast<Logger::Level>(0), msg);
}

void log_info(const std::string &msg)
{
    if (g_cli_logger) g_cli_logger->log(static_cast<Logger::Level>(1), msg);
}

void log_warn(const std::string &msg)
{
    if (g_cli_logger) g_cli_logger->log(static_cast<Logger::Level>(2), msg);
}

void log_error(const std::string &msg)
{
    if (g_cli_logger) g_cli_logger->log(static_cast<Logger::Level>(3), msg);
}

void print_help()
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
              << "  .export <path>  Export database to SQL, path can be dir or file\n"
              << "  .import <file>  Import SQL file to database\n"
              << "Enter SQL terminated by ';' to run.\n";
}

int count_substring(const std::string &s, const std::string &pat)
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

std::string trim_copy(const std::string &s)
{
    size_t start = 0, end = s.size();
    while (start < end && std::isspace(static_cast<unsigned char>(s[start]))) start++;
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) end--;
    return s.substr(start, end - start);
}

std::string strip_quotes_copy(const std::string &s)
{
    if (s.size() >= 2)
    {
        char first = s.front();
        char last = s.back();
        if ((first == '"' && last == '"') || (first == '\'' && last == '\''))
            return s.substr(1, s.size() - 2);
    }
    return s;
}

std::string resolve_export_output_path(const std::string &input)
{
    using namespace std::filesystem;
    std::string raw = strip_quotes_copy(trim_copy(input));
    if (raw.empty()) return raw;

    std::error_code ec;
    path p(raw);
    bool existsPath = exists(p, ec);
    bool isDirExisting = existsPath && is_directory(p, ec);

    auto ends_with_sep = [&raw]() {
#ifdef _WIN32
        if (raw.empty()) return false;
        char c = raw.back();
        return c == '/' || c == '\\';
#else
        return !raw.empty() && raw.back() == '/';
#endif
    }();

    bool treatAsDir = isDirExisting;
    if (!existsPath)
    {
        if (ends_with_sep)
        {
            treatAsDir = true;
        }
        else
        {
            path filename = p.filename();
            if (filename.has_extension() == false)
            {
                treatAsDir = true;
            }
        }
    }

    if (treatAsDir)
    {
        std::time_t t = std::time(nullptr);
        std::tm tmv{};
#ifdef _WIN32
        localtime_s(&tmv, &t);
#else
        localtime_r(&t, &tmv);
#endif
        char buf[32];
        std::snprintf(buf, sizeof(buf), "dump_%04d%02d%02d_%02d%02d%02d.sql",
            tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday, tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
        create_directories(p, ec);
        path full = p / buf;
        return absolute(full, ec).string();
    }

    path parent = p.parent_path();
    if (!parent.empty() && !exists(parent, ec))
    {
        create_directories(parent, ec);
    }

    return absolute(p, ec).string();
}

std::string make_prompt(minidb::AuthService *auth)
{
    if (auth && auth->isLoggedIn())
    {
        return auth->getCurrentUser() + ":" + role_to_cn(auth->getCurrentUserRoleString()) + "] ";
    }
    return std::string();
}

bool require_exec_mode(bool doExec, const std::string &msg)
{
    if (doExec) return true;
    std::cout << msg << std::endl;
    return false;
}

bool require_logged_in(minidb::AuthService *auth)
{
    if (auth && auth->isLoggedIn()) return true;
    std::cout << "[权限拒绝] 请先登录 (.login)" << std::endl;
    return false;
}

bool require_dba(minidb::AuthService *auth)
{
    if (!require_logged_in(auth)) return false;
    if (auth->isDBA()) return true;
    std::cout << "[权限拒绝] 需要DBA权限" << std::endl;
    return false;
}

} // namespace cli
} // namespace minidb


