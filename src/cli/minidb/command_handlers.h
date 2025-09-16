#pragma once

#include <string>

namespace minidb {

class AuthService;
class Catalog;
class Executor;
class StorageEngine;

namespace cli {

// 处理内建命令，返回是否匹配并已处理
bool handle_help(const std::string &line);
bool handle_login(const std::string &line, AuthService *auth);
bool handle_logout(const std::string &line, AuthService *auth);
bool handle_info(const std::string &line, AuthService *auth);
bool handle_users(const std::string &line, AuthService *auth);
bool handle_dump(const std::string &line, Catalog *catalog, StorageEngine *se);
bool handle_export_cmd(const std::string &line, Catalog *catalog, StorageEngine *se);
bool handle_import_cmd(const std::string &line, Executor *exec, Catalog *catalog);
// 调试：全盘扫描一个表（忽略页链），打印行数
bool handle_debug_fullscan(const std::string &line, Catalog *catalog, StorageEngine *se);
// 调试：设置表的首页ID
bool handle_debug_set_firstpage(const std::string &line, Catalog *catalog);
// 调试：猜测表的链表首页（通过 next_link 反推头结点）
bool handle_debug_guess_firstpage(const std::string &line, Catalog *catalog, StorageEngine *se);

} // namespace cli
} // namespace minidb


