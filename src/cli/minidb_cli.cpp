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
              << "  .exit           Quit\n"
              << "  .dump <file>    Export database to SQL file\n"
              << "  .import <file>  Import SQL file to database\n"
              << "Enter SQL terminated by ';' to run.\n";
}

using minidb::cliutil::autocorrect_leading_keyword;
using minidb::cliutil::can_terminate_without_semicolon;

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

    std::string buffer;
    buffer.reserve(1024);

    while (true)
    {
        std::cout << ">> " << std::flush;
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
        
        // 处理导入导出命令
        if (line.substr(0, 6) == ".dump ")
        {
            if (!doExec) {
                std::cerr << "Error: Export requires execution mode. Use --exec flag." << std::endl;
                continue;
            }
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
    return 0;
}
