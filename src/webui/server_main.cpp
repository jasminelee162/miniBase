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
            // 3.5) 基本权限校验（至少拦截 SELECT __users__）
            try {
                if (plan && plan->type == PlanType::SeqScan) {
                    if (!auth.checkTablePermission(plan->table_name, Permission::SELECT)) {
                        throw std::runtime_error(std::string("Permission denied: SELECT on ") + plan->table_name);
                    }
                }
            } catch (const std::exception& e) {
                out["error"] = e.what();
                res.headers["Content-Type"] = "application/json";
                res.body = out.dump();
                return;
            }

            // 4) 执行
            auto exec = std::make_unique<Executor>(se);
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
        } catch (const std::exception& e) {
            out["error"] = e.what();
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


