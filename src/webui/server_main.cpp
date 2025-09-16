#include "../util/json.hpp"
#include "../storage/storage_engine.h"
#include "../catalog/catalog.h"
#include "../auth/auth_service.h"
#include "../sql_compiler/lexer/lexer.h"
#include "../sql_compiler/parser/parser.h"
#include "../sql_compiler/parser/ast_json_serializer.h"
#include "../sql_compiler/semantic/semantic_analyzer.h"
#include "../frontend/translator/json_to_plan.h"
#include "../engine/executor/executor.h"
#include "httplib.h"
#include "../tools/sql_dump/sql_dumper.h"
#include "../tools/sql_import/sql_importer.h"
#include <memory>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <csignal>
#ifdef _WIN32
#include <windows.h>
#include <crtdbg.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#endif

using json = nlohmann::json;
using namespace minidb;

static void configure_no_crash_dialogs() {
#ifdef _WIN32
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    // 关闭断言弹窗，并将输出重定向到标准错误
    _CrtSetReportMode(_CRT_ASSERT, 0);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
#endif
}

int main() {
    configure_no_crash_dialogs();
    std::signal(SIGABRT, [](int){ std::cerr << "FATAL: abort called (SIGABRT)" << std::endl; });
    // 选择可写且存在的数据库路径：优先使用项目根目录下的 data/mini.db
    std::string dbFile = "data/mini.db";
#ifdef PROJECT_ROOT_DIR
    dbFile = std::string(PROJECT_ROOT_DIR) + "/data/mini.db";
#endif
    // 确保父目录存在
    try {
        std::filesystem::path dbPath(dbFile);
        if (dbPath.has_parent_path()) {
            std::error_code ec;
            std::filesystem::create_directories(dbPath.parent_path(), ec);
        }
    } catch (...) {
        // 忽略目录创建异常，后续由 DiskManager 再报错
    }
    auto se = std::make_shared<StorageEngine>(dbFile, 16);
    auto catalog = std::make_shared<Catalog>(se.get());
    AuthService auth(se.get(), catalog.get());

    minihttplib::Server svr;

    // 登录
    svr.Post("/auth/login", [&auth](const minihttplib::Request& req, minihttplib::Response& res){
        try {
            auto j = json::parse(req.body);
            std::string u = j.value("username", "");
            std::string p = j.value("password", "");
            bool ok = auth.login(u, p);
            json out;
            out["ok"] = ok;
            if (ok) {
                out["user"] = auth.getCurrentUser();
                out["role"] = auth.getCurrentUserRoleString();
            }
            res.headers["Content-Type"] = "application/json";
            res.body = out.dump();
        } catch (...) {
            res.status = 400;
            res.body = "{}";
        }
    });

    // 当前登录信息
    svr.Get("/auth/me", [&auth](const minihttplib::Request&, minihttplib::Response& res){
        json out;
        bool ok = auth.isLoggedIn();
        out["ok"] = ok;
        if (ok) {
            out["user"] = auth.getCurrentUser();
            out["role"] = auth.getCurrentUserRoleString();
        }
        res.headers["Content-Type"] = "application/json";
        res.body = out.dump();
    });

    // 登出
    svr.Post("/auth/logout", [&auth](const minihttplib::Request&, minihttplib::Response& res){
        auth.logout();
        json out; out["ok"] = true; res.headers["Content-Type"] = "application/json"; res.body = out.dump();
    });

    // 创建用户（仅DBA）
    svr.Post("/auth/createUser", [&auth](const minihttplib::Request& req, minihttplib::Response& res){
        json out;
        try {
            auto j = json::parse(req.body);
            std::string u = j.value("username", "");
            std::string p = j.value("password", "");
            std::string r = j.value("role", "ANALYST");
            std::string ru; ru.resize(r.size());
            std::transform(r.begin(), r.end(), ru.begin(), [](unsigned char c){ return static_cast<char>(std::toupper(c)); });
            Role role = Role::ANALYST;
            if (ru == "DBA") role = Role::DBA; else if (ru == "DEVELOPER") role = Role::DEVELOPER; else role = Role::ANALYST;
            bool ok = auth.createUser(u, p, role);
            out["ok"] = ok;
            if (!ok) { res.status = 403; }
        } catch (...) {
            res.status = 400; out["ok"] = false;
        }
        res.headers["Content-Type"] = "application/json";
        res.body = out.dump();
    });

    // 列出所有用户（仅DBA）
    svr.Get("/auth/listUsers", [&auth](const minihttplib::Request&, minihttplib::Response& res){
        json out;
        if (!auth.isLoggedIn() || !auth.isDBA()) {
            res.status = 403; out["ok"] = false; res.body = out.dump(); return;
        }
        auto users = auth.listUsers();
        out["ok"] = true;
        out["users"] = users;
        res.headers["Content-Type"] = "application/json";
        res.body = out.dump();
    });

    // 删除用户（仅DBA）
    svr.Post("/auth/deleteUser", [&auth](const minihttplib::Request& req, minihttplib::Response& res){
        json out;
        try {
            if (!auth.isLoggedIn() || !auth.isDBA()) { res.status = 403; out["ok"]=false; res.body = out.dump(); return; }
            auto j = json::parse(req.body);
            std::string u = j.value("username", "");
            if (u == "root" || u.empty()) { res.status = 400; out["ok"]=false; res.body = out.dump(); return; }
            bool ok = auth.deleteUser(u);
            out["ok"] = ok;
            res.headers["Content-Type"] = "application/json";
            res.body = out.dump();
        } catch (...) { res.status = 400; out["ok"]=false; res.body = out.dump(); }
    });

    // 导出数据库到 SQL（仅DBA）
    svr.Post("/db/dump", [&se, &catalog, &auth](const minihttplib::Request& req, minihttplib::Response& res){
        json out;
        try {
            if (!auth.isLoggedIn() || !auth.isDBA()) { res.status = 403; out["ok"]=false; res.body = out.dump(); return; }
            auto j = json::parse(req.body);
            std::string filename = j.value("filename", "dump.sql");
            SQLDumper dumper(catalog.get(), se.get());
            bool ok = dumper.DumpToFile(filename, DumpOption::StructureAndData);
            out["ok"] = ok; out["file"] = filename;
            res.headers["Content-Type"] = "application/json";
            res.body = out.dump();
        } catch (...) { res.status = 400; out["ok"]=false; res.body = out.dump(); }
    });

    // 从 SQL 导入（仅DBA）
    svr.Post("/db/import", [&se, &catalog, &auth](const minihttplib::Request& req, minihttplib::Response& res){
        json out;
        try {
            if (!auth.isLoggedIn() || !auth.isDBA()) { res.status = 403; out["ok"]=false; res.body = out.dump(); return; }
            auto j = json::parse(req.body);
            std::string filename = j.value("filename", "");
            if (filename.empty()) { res.status = 400; out["ok"]=false; res.body = out.dump(); return; }
            auto exec = std::make_unique<Executor>(se);
            exec->SetCatalog(catalog);
            SQLImporter importer(exec.get(), catalog.get());
            bool ok = importer.ImportSQLFile(filename);
            out["ok"] = ok;
            res.headers["Content-Type"] = "application/json";
            res.body = out.dump();
        } catch (...) { res.status = 400; out["ok"]=false; res.body = out.dump(); }
    });

    // 执行SQL（沿用 CLI 的解析→语义→AST→JSON→Plan→执行）
    svr.Post("/sql/execute", [&se, &catalog, &auth](const minihttplib::Request& req, minihttplib::Response& res){
        json out;
        try {
            auto in = json::parse(req.body);
            std::string sql = in.value("sql", "");
            if (sql.empty()) throw std::runtime_error("empty sql");

            // 1) 词法→语法
            Lexer lexer(sql);
            auto tokens = lexer.tokenize();
            Parser parser(tokens);
            auto stmt = parser.parse();
            // 2) 语义
            SemanticAnalyzer sem; sem.setCatalog(catalog.get());
            sem.analyze(stmt.get());
            // 3) AST→JSON→Plan
            auto j = ASTJson::toJson(stmt.get());
            auto plan = JsonToPlan::translate(j);
            // 4) 执行
            // 附加权限检查器，确保 WebUI 与 CLI 一致的表级可见性/权限控制
            PermissionChecker checker(&auth);
            auto exec = std::make_unique<Executor>(catalog.get(), &checker);
            exec->SetAuthService(&auth);
            exec->SetStorageEngine(se);
            exec->SetCatalog(catalog);
            auto rows = exec->execute(plan.get());

            // 推断列名（从第一行的 ColumnValue 列名获取）
            std::vector<std::string> columns;
            if (!rows.empty()) {
                for (const auto& cv : rows.front().columns) {
                    columns.push_back(cv.col_name);
                }
            }
            out["columns"] = columns;
            out["rows"] = json::array();
            for (const auto& r : rows) {
                json jr = json::array();
                for (const auto& cv : r.columns) jr.push_back(cv.value);
                out["rows"].push_back(jr);
            }
            if (rows.empty()) {
                // 为无返回行的语句填充友好消息
                out["message"] = std::string("OK (no rows)");
            } else {
                out["message"] = std::string("OK");
            }
        } catch (const std::exception& e) {
            out["error"] = e.what();
            res.status = 500;
        } catch (...) {
            out["error"] = "unknown error";
            res.status = 500;
        }
        res.headers["Content-Type"] = "application/json";
        res.body = out.dump();
    });

    // 手动刷盘
    svr.Get("/flush", [&se](const minihttplib::Request&, minihttplib::Response& res){
        try { se->Checkpoint(); res.body = "flushed"; }
        catch (...) { res.status = 500; res.body = "flush error"; }
    });

    // 静态首页（在多种可能路径中查找）
    svr.Get("/", [](const minihttplib::Request&, minihttplib::Response& res){
        auto read_file = [](const std::string& path, std::string& out) -> bool {
            std::ifstream f(path, std::ios::binary);
            if (!f.is_open()) return false;
            std::ostringstream ss; ss << f.rdbuf();
            out = ss.str();
            return true;
        };
        std::string html;
        // 搜索路径优先级：运行目录 → 构建目录的父级源码路径 → 定义的项目根目录
        std::vector<std::string> candidates = {
            "index.html",
            "./index.html",
            "../src/webui/index.html",
            "../../src/webui/index.html",
            "src/webui/index.html"
        };
#ifdef PROJECT_ROOT_DIR
        candidates.insert(candidates.begin(), std::string(PROJECT_ROOT_DIR) + "/src/webui/index.html");
#endif
        bool found = false;
        for (const auto& p : candidates) {
            if (read_file(p, html)) { found = true; break; }
        }
        if (found) {
            res.headers["Content-Type"] = "text/html; charset=utf-8";
            res.body = html;
        } else {
            res.status = 404;
            res.body = "index not found";
        }
    });

    // 简单健康检查
    svr.Get("/ping", [](const minihttplib::Request&, minihttplib::Response& res){ res.body = "pong"; });

    // 避免浏览器自动请求 favicon 报 404
    svr.Get("/favicon.ico", [](const minihttplib::Request&, minihttplib::Response& res){ res.status = 204; });

    // 列出 logs 目录中的文件
    svr.Get("/logs/list", [](const minihttplib::Request&, minihttplib::Response& res){
        json out;
        try {
            std::vector<std::string> files;
#ifdef PROJECT_ROOT_DIR
            std::string dir = std::string(PROJECT_ROOT_DIR) + "/logs";
#else
            std::string dir = "logs";
#endif
            for (auto& p : std::filesystem::directory_iterator(dir)) {
                if (p.is_regular_file()) files.push_back(p.path().filename().string());
            }
            out["ok"] = true; out["files"] = files; res.headers["Content-Type"] = "application/json"; res.body = out.dump();
        } catch (...) { res.status = 500; out["ok"]=false; res.body = out.dump(); }
    });

    // 读取某个日志文件内容（POST JSON: {"file":"xxx.log"}）
    svr.Post("/logs/read", [](const minihttplib::Request& req, minihttplib::Response& res){
        json out;
        try {
            auto j = json::parse(req.body);
            std::string name = j.value("file", "");
            if (name.empty()) { res.status = 400; out["ok"]=false; res.body = out.dump(); return; }
#ifdef PROJECT_ROOT_DIR
            std::string path = std::string(PROJECT_ROOT_DIR) + "/logs/" + name;
#else
            std::string path = std::string("logs/") + name;
#endif
            std::ifstream f(path, std::ios::binary);
            if (!f.is_open()) { res.status = 404; out["ok"]=false; res.body = out.dump(); return; }
            std::ostringstream ss; ss << f.rdbuf();
            out["ok"] = true; out["content"] = ss.str();
            res.headers["Content-Type"] = "application/json"; res.body = out.dump();
        } catch (...) { res.status = 500; out["ok"]=false; res.body = out.dump(); }
    });

    // AI 对接：读取 ai/endpoint.txt 指定的 HTTP 接口，转发 prompt
    svr.Post("/ai/complete", [](const minihttplib::Request& req, minihttplib::Response& res){
        json out;
        try {
            auto j = json::parse(req.body);
            std::string prompt = j.value("prompt", "");
            if (prompt.empty()) { res.status = 400; out["ok"]=false; out["error"]="empty prompt"; res.body = out.dump(); return; }

            // 读取 endpoint 配置（优先级：环境变量 → ai/endpoint.txt → ai/config.json(endpoint) → config.json(ai.endpoint)）
            auto getenv_str = [](const char* k) -> std::string { const char* v = std::getenv(k); return v ? std::string(v) : std::string(); };
            std::string url = getenv_str("MINIDB_AI_ENDPOINT");
            auto try_read_text = [](const std::string& p, std::string& out_text) -> bool {
                std::ifstream f(p, std::ios::binary);
                if (!f.is_open()) return false;
                std::ostringstream ss; ss << f.rdbuf();
                out_text = ss.str();
                // trim
                while (!out_text.empty() && (out_text.back()=='\n' || out_text.back()=='\r' || out_text.back()==' ')) out_text.pop_back();
                return true;
            };
            auto try_read_json_key = [](const std::string& p, const std::string& key, std::string& out_val) -> bool {
                std::ifstream f(p, std::ios::binary);
                if (!f.is_open()) return false;
                try { json j; f >> j; if (j.contains(key)) { out_val = j[key].get<std::string>(); return true; } } catch (...) {}
                return false;
            };

            if (url.empty()) {
#ifdef PROJECT_ROOT_DIR
                std::vector<std::string> candidates = {
                    std::string(PROJECT_ROOT_DIR) + "/ai/endpoint.txt",
                    std::string(PROJECT_ROOT_DIR) + "/ai/config.json",
                    std::string(PROJECT_ROOT_DIR) + "/config.json",
                    "ai/endpoint.txt",
                    "ai/config.json",
                    "config.json"
                };
#else
                std::vector<std::string> candidates = {
                    "ai/endpoint.txt",
                    "ai/config.json",
                    "config.json"
                };
#endif
                std::string tmp;
                // endpoint.txt
                if (try_read_text(candidates[0], tmp) && !tmp.empty()) { url = tmp; }
                // ai/config.json: {"endpoint":"..."}
                if (url.empty() && candidates.size() > 1 && try_read_json_key(candidates[1], "endpoint", tmp)) { url = tmp; }
                // config.json: {"ai":{"endpoint":"..."}}
                if (url.empty() && candidates.size() > 2) {
                    std::ifstream f(candidates[2], std::ios::binary);
                    if (f.is_open()) {
                        try { json j; f >> j; if (j.contains("ai") && j["ai"].contains("endpoint")) url = j["ai"]["endpoint"].get<std::string>(); } catch (...) {}
                    }
                }
                // fallback: relative files
                if (url.empty() && candidates.size() > 3 && try_read_text(candidates[3], tmp) && !tmp.empty()) url = tmp;
                if (url.empty() && candidates.size() > 4 && try_read_json_key(candidates[4], "endpoint", tmp)) url = tmp;
                if (url.empty() && candidates.size() > 5) {
                    std::ifstream f(candidates[5], std::ios::binary);
                    if (f.is_open()) { try { json j; f >> j; if (j.contains("ai") && j["ai"].contains("endpoint")) url = j["ai"]["endpoint"].get<std::string>(); } catch (...) {} }
                }
            }

            if (url.empty()) { res.status = 500; out["ok"]=false; out["error"]="AI endpoint not configured"; res.body = out.dump(); return; }

            // 解析 url: 仅支持 http://host[:port]/path
            auto ensure_http = [](const std::string& s){ return s.rfind("http://", 0) == 0; };
            if (!ensure_http(url)) { res.status = 500; out["ok"]=false; out["error"]="only http:// supported"; res.body = out.dump(); return; }
            std::string rest = url.substr(7);
            size_t slash = rest.find('/');
            std::string hostport = slash==std::string::npos ? rest : rest.substr(0, slash);
            std::string path = slash==std::string::npos ? "/" : rest.substr(slash);
            std::string host = hostport;
            int port = 80;
            size_t colon = hostport.find(':');
            if (colon != std::string::npos) { host = hostport.substr(0, colon); port = std::stoi(hostport.substr(colon+1)); }

            // 通过 WinHTTP 发送 POST（Windows）
            std::string resp_body;
#ifdef _WIN32
            auto utf16 = [](const std::string& s){
                int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
                std::wstring w; w.resize(len);
                MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &w[0], len);
                return w;
            };
            std::wstring whost = utf16(host);
            std::wstring wpath = utf16(path);
            std::string body = json({{"prompt", prompt}}).dump();
            HINTERNET hSession = WinHttpOpen(L"MiniDB/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
            if (!hSession) { res.status = 502; out["ok"]=false; out["error"]="winhttp open failed"; res.body = out.dump(); return; }
            HINTERNET hConnect = WinHttpConnect(hSession, whost.c_str(), (INTERNET_PORT)port, 0);
            if (!hConnect) { WinHttpCloseHandle(hSession); res.status = 502; out["ok"]=false; out["error"]="winhttp connect failed"; res.body = out.dump(); return; }
            HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", wpath.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
            if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); res.status = 502; out["ok"]=false; out["error"]="winhttp openrequest failed"; res.body = out.dump(); return; }
            std::wstring hdrs = L"Content-Type: application/json\r\n";
            BOOL b = WinHttpSendRequest(hRequest, hdrs.c_str(), (DWORD)hdrs.size(), (LPVOID)body.data(), (DWORD)body.size(), (DWORD)body.size(), 0);
            if (!b) { WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); res.status = 502; out["ok"]=false; out["error"]="winhttp send failed"; res.body = out.dump(); return; }
            b = WinHttpReceiveResponse(hRequest, NULL);
            if (!b) { WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); res.status = 502; out["ok"]=false; out["error"]="winhttp recv failed"; res.body = out.dump(); return; }
            DWORD dwSize = 0;
            do {
                if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) break;
                if (dwSize == 0) break;
                std::string buf; buf.resize(dwSize);
                DWORD dwRead = 0;
                if (!WinHttpReadData(hRequest, &buf[0], dwSize, &dwRead)) break;
                resp_body.append(buf.data(), dwRead);
            } while (dwSize > 0);
            WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
#else
            res.status = 500; out["ok"]=false; out["error"]="AI client unsupported on this platform"; res.body = out.dump(); return;
#endif
            try {
                auto bj = json::parse(resp_body);
                out["ok"] = true;
                out["reply"] = bj.value("reply", resp_body);
            } catch (...) {
                out["ok"] = true; out["reply"] = resp_body;
            }
            res.headers["Content-Type"] = "application/json"; res.body = out.dump();
        } catch (...) { res.status = 400; out["ok"]=false; res.body = out.dump(); }
    });

    try {
        std::cout << "MiniDB WebUI server (mock) started on http://127.0.0.1:8080" << std::endl;
        svr.listen("0.0.0.0", 8080);
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] 未捕获异常: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "[ERROR] 未知致命错误" << std::endl;
        return 1;
    }
}


