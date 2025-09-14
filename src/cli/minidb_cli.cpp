#include "../sql_compiler/lexer/lexer.h"
#include "../sql_compiler/parser/parser.h"
#include "../sql_compiler/parser/ast_json_serializer.h"
#include "../sql_compiler/semantic/semantic_analyzer.h"
#include "../sql_compiler/planner/plan_printer.h"
#include "../frontend/translator/json_to_plan.h"
#include "../storage/storage_engine.h"
#include "../engine/executor/executor.h"
#include "../catalog/catalog.h"
#include <iostream>
#include <string>
#include <memory>
#include <fstream>
#ifdef _WIN32
#include <windows.h>
#endif

static void print_help() {
    std::cout << "MiniDB CLI\n"
              << "Usage: minidb_cli [--exec|--json] [--db <path>] [--catalog <path>]\\n"
              << "Commands:\n"
              << "  .help           Show this help\n"
              << "  .exit           Quit\n"
              << "Enter SQL terminated by ';' to run.\n";
}

int main(int argc, char** argv) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    bool doExec = true;            // default execute
    bool outputJsonOnly = false;   // --json overrides
    std::string dbFile = "data/mini.db"; //文件存储位置
    std::string catalogFile = "catalog.dat";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--exec") { doExec = true; outputJsonOnly = false; }
        else if (arg == "--json") { outputJsonOnly = true; doExec = false; }
        else if (arg == "--db" && i + 1 < argc) { dbFile = argv[++i]; }
        else if (arg == "--catalog" && i + 1 < argc) { catalogFile = argv[++i]; }
        else if (arg == "-h" || arg == "--help") { print_help(); return 0; }
        else {
            std::cerr << "Unknown argument: " << arg << std::endl;
            print_help();
            return 1;
        }
    }

    std::shared_ptr<minidb::StorageEngine> se;
    std::shared_ptr<minidb::Catalog> catalog;
    std::unique_ptr<minidb::Executor> exec;
    if (doExec) {
        se = std::make_shared<minidb::StorageEngine>(dbFile, 16);
        catalog = std::make_shared<minidb::Catalog>(se.get());
        // catalog->SetQuiet(true); // 方法不存在，注释掉
        exec = std::make_unique<minidb::Executor>(se);
        exec->SetCatalog(catalog);
    }

    std::cout << "MiniDB CLI ready. Type .help for help.\n";

    std::string buffer;
    buffer.reserve(1024);

    while (true) {
        std::cout << ">> " << std::flush;
        std::string line;
        if (!std::getline(std::cin, line)) break;
        if (line == ".exit") break;
        if (line == ".help") { print_help(); continue; }
        if (line.empty()) continue;

        buffer += line;
        // accumulate until ;
        if (buffer.find(';') == std::string::npos) {
            buffer += '\n';
            continue;
        }

        try {
            Lexer l(buffer);
            auto tokens = l.tokenize();
            Parser p(tokens);
            auto stmt = p.parse();

            // optional semantic checks
            try {
                SemanticAnalyzer sem;
                sem.setCatalog(catalog.get());  // 使用CLI主程序的同一个Catalog实例
                sem.analyze(stmt.get());
            } catch (const std::exception &e) {
                std::cerr << "[Semantic][WARN] " << e.what() << std::endl;
            }

            auto j = ASTJson::toJson(stmt.get());
            std::cout << "astjson: " << j.dump(2) << std::endl;

            if (outputJsonOnly) {
                std::cout << "只有astjson: "<<j.dump(2) << std::endl;
            } else if (doExec) {
                auto plan = JsonToPlan::translate(j);
                // std::cout << "plan: " << plan->toString() << std::endl;
                // 调用 PlanPrinter 打印
                PlanPrinter printer;
                std::cout << "plan: " << printer.print(plan.get()) << std::endl;

                exec->execute(plan.get());
                std::cout << "[OK] executed." << std::endl;

                // 添加文件检查
                std::ifstream file(dbFile, std::ios::binary | std::ios::ate);
                if (file.is_open()) {
                    auto size = file.tellg();
                    std::cout << "[INFO] 数据库文件大小: " << size << " 字节" << std::endl;
                    file.close();
                }
            }
        } catch (const ParseError &e) {
            std::cerr << "[Parser][ERROR] " << e.what() << " at (" << e.getLine() << "," << e.getColumn() << ")" << std::endl;
        } catch (const std::exception &e) {
            std::cerr << "[ERROR] " << e.what() << std::endl;
        }

        buffer.clear();
    }

    std::cout << "Bye." << std::endl;
    return 0;
} 