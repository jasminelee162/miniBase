#pragma once

#include <string>

namespace minidb {

class AuthService;

namespace cli {

// 打印当前用户信息（中文）
void print_user_info(AuthService *auth);

// 进入用户管理交互循环（仅 DBA）
void manage_users(AuthService *auth);

// 角色英文标识转中文（DBA/DEVELOPER/ANALYST）
std::string role_to_cn(const std::string &roleStr);

} // namespace cli
} // namespace minidb


