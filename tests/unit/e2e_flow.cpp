#include "../../src/sql_compiler/lexer/lexer.h"
#include "../../src/sql_compiler/parser/parser.h"
#include "../../src/sql_compiler/parser/ast_json_serializer.h"
#include "../../src/sql_compiler/semantic/semantic_analyzer.h"
#include <iostream>
#include <csignal>
#ifdef _WIN32
#include <windows.h>
#endif

int main()
{
    // enable UTF-8 output on Windows console to avoid Chinese garbling
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    std::cout << "Running e2e flow test..." << std::endl;

    // install simple handlers to diagnose abort/uncaught exceptions
    std::signal(SIGABRT, [](int){
        std::cerr << "FATAL: SIGABRT received (abort called)" << std::endl;
    });

    try {

    // Example SQLs
    std::vector<std::string> sqls = {
        "CREATE TABLE users(id INT, name VARCHAR, age INT);",
        "INSERT INTO users(id,name,age) VALUES (1,'Alice',20);",
        "SELECT id,name FROM users WHERE age > 18;",
        "DELETE FROM users WHERE id = 1;"
    };

    for (auto &sql : sqls) {
        std::cout << "SQL: " << sql << std::endl;

        std::cout << "[Lexer]" << std::endl;
        Lexer l(sql);
        auto tokens = l.tokenize();

        std::cout << "[Parser]" << std::endl;
        Parser p(tokens);
        auto stmt = p.parse();

        std::cout << "[Semantic]" << std::endl;
        // run semantic analyzer; catch semantic errors so test continues
        try {
            SemanticAnalyzer sem;
            sem.analyze(stmt.get());
        }
        catch (const std::exception &e) {
            std::cerr << "[Semantic][ERROR] " << e.what() << std::endl;
            // continue to JSON output even if semantic check failed
        }

        std::cout << "[JSON]" << std::endl;
        auto j = ASTJson::toJson(stmt.get());
        std::cout << j.dump(2) << std::endl;
    }
        std::cout << "e2e finished" << std::endl;
        return 0;
    }
    catch (const std::exception &e) {
        std::cerr << "UNCAUGHT std::exception: " << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "UNCAUGHT unknown exception (possible abort)" << std::endl;
        return 1;
    }
}
