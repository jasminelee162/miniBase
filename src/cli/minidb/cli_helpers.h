#pragma once

#include <string>
#include "../auth/auth_service.h"

namespace minidb {
namespace cli {

// 显示 CLI 帮助
void print_help();

// 统计子串出现次数（不重叠）
int count_substring(const std::string &s, const std::string &pat);

// 去除首尾空白副本
std::string trim_copy(const std::string &s);

// 去除首尾引号副本
std::string strip_quotes_copy(const std::string &s);

// 解析导出路径：可传入目录或文件；必要时创建父目录
std::string resolve_export_output_path(const std::string &input);

// 构造提示符：已登录则返回 "user:角色] "，否则返回空串
std::string make_prompt(minidb::AuthService *auth);

// 需求：执行模式
// doExec==true 返回true，否则打印消息并返回false
bool require_exec_mode(bool doExec, const std::string &msg);

// 需求：已登录
// 已登录返回true，否则打印统一消息并返回false
bool require_logged_in(minidb::AuthService *auth);

// 需求：DBA 权限
bool require_dba(minidb::AuthService *auth);

} // namespace cli
} // namespace minidb


