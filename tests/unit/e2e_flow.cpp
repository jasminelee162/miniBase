#include "../../src/sql_compiler/lexer/lexer.h"
#include "../../src/sql_compiler/parser/parser.h"
#include "../../src/sql_compiler/parser/ast_json_serializer.h"
#include "../../src/sql_compiler/semantic/semantic_analyzer.h"
#include "../../src/frontend/translator/json_to_plan.h"
#include "../../src/storage/storage_engine.h"
#include "../../src/engine/executor/executor.h"
#include "../../src/catalog/catalog.h"
#include <iostream>
#include <csignal>
#include <memory>
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

    // 在循环外初始化存储引擎和执行器（避免重复创建）
    std::cout << "[Storage] Initializing storage engine..." << std::endl;
    auto se = std::make_shared<minidb::StorageEngine>("data/mini.db", 16);
    
    std::cout << "[Executor] Initializing executor..." << std::endl;
    minidb::Executor exec(se);
    
    std::cout << "[Catalog] Loading catalog..." << std::endl;
    auto catalog = std::make_shared<minidb::Catalog>("catalog.dat");
    exec.SetCatalog(catalog);

    // construct semantic analyzer once to avoid repeated Catalog::Load()
    SemanticAnalyzer sem;

    // enhanced AST printer
    struct ASTPrinter : public ASTVisitor {
        // helper to print data type
        std::string dtypeToString(ColumnDefinition::DataType dt) {
            switch (dt) {
                case ColumnDefinition::DataType::INT: return "INT";
                case ColumnDefinition::DataType::VARCHAR: return "VARCHAR";
                default: return "UNKNOWN";
            }
        }

        void visit(LiteralExpression& expr) override {
            std::cout << "Literal(" << expr.getValue() << ")";
        }
        void visit(IdentifierExpression& expr) override {
            std::cout << "Identifier(" << expr.getName() << ")";
        }
        void visit(BinaryExpression& expr) override {
            std::cout << "Binary(";
            if (expr.getLeft()) expr.getLeft()->accept(*this); else std::cout<<"null";
            std::cout << " ";
            // print operator
            switch (expr.getOperator()){
                case BinaryExpression::Operator::EQUALS: std::cout<<"="; break;
                case BinaryExpression::Operator::LESS_THAN: std::cout<<"<"; break;
                case BinaryExpression::Operator::GREATER_THAN: std::cout<<">"; break;
                case BinaryExpression::Operator::LESS_EQUAL: std::cout<<"<="; break;
                case BinaryExpression::Operator::GREATER_EQUAL: std::cout<<">="; break;
                case BinaryExpression::Operator::NOT_EQUAL: std::cout<<"!="; break;
                case BinaryExpression::Operator::PLUS: std::cout<<"+"; break;
                case BinaryExpression::Operator::MINUS: std::cout<<"-"; break;
                case BinaryExpression::Operator::MULTIPLY: std::cout<<"*"; break;
                case BinaryExpression::Operator::DIVIDE: std::cout<<"/"; break;
                default: std::cout<<"?"; break;
            }
            std::cout << " ";
            if (expr.getRight()) expr.getRight()->accept(*this); else std::cout<<"null";
            std::cout << ")";
        }

        void visit(CreateTableStatement& stmt) override {
            std::cout << "CreateTable(" << stmt.getTableName() << ", schema=[";
            const auto &cols = stmt.getColumns();
            for (size_t i = 0; i < cols.size(); ++i) {
                const auto &c = cols[i];
                std::cout << c.getName() << ":" << dtypeToString(c.getType());
                if (i + 1 < cols.size()) std::cout << ", ";
            }
            std::cout << "])";
        }

        void visit(InsertStatement& stmt) override {
            std::cout << "Insert(" << stmt.getTableName() << ", cols=[";
            const auto &cols = stmt.getColumnNames();
            for (size_t i = 0; i < cols.size(); ++i) {
                std::cout << cols[i];
                if (i + 1 < cols.size()) std::cout << ", ";
            }
            std::cout << "], values=[";
            const auto &vls = stmt.getValueLists();
            for (size_t i = 0; i < vls.size(); ++i) {
                std::cout << "[";
                const auto &vals = vls[i].getValues();
                for (size_t j = 0; j < vals.size(); ++j) {
                    if (vals[j]) vals[j]->accept(*this); else std::cout<<"null";
                    if (j + 1 < vals.size()) std::cout << ", ";
                }
                std::cout << "]";
                if (i + 1 < vls.size()) std::cout << ", ";
            }
            std::cout << "])";
        }

        void visit(SelectStatement& stmt) override {
            std::cout << "Select(" << stmt.getTableName() << ", columns=[";
            const auto &cols = stmt.getColumns();
            for (size_t i = 0; i < cols.size(); ++i) {
                std::cout << cols[i];
                if (i + 1 < cols.size()) std::cout << ", ";
            }
            std::cout << "]";
            if (stmt.getWhereClause()) {
                std::cout << ", where=";
                stmt.getWhereClause()->accept(*this);
            }
            std::cout << ")";
        }

        void visit(DeleteStatement& stmt) override {
            std::cout << "Delete(" << stmt.getTableName();
            if (stmt.getWhereClause()) {
                std::cout << ", where=";
                stmt.getWhereClause()->accept(*this);
            }
            std::cout << ")";
        }
    } printer;

    for (auto &sql : sqls) {
        std::cout << "\n=== Processing SQL: " << sql << " ===" << std::endl;

        std::cout << "[Lexer] Tokenizing..." << std::endl;
        Lexer l(sql);
        auto tokens = l.tokenize();

        // print tokens
        for (const auto &t : tokens) {
            std::cout << "  token: " << static_cast<int>(t.type) << " '" << t.lexeme << "' (" << t.line << "," << t.column << ")";
            if (!t.errorMessage.empty()) std::cout << " ERROR:" << t.errorMessage;
            std::cout << std::endl;
        }

        std::cout << "[Parser] Parsing..." << std::endl;
        Parser p(tokens);
        auto stmt = p.parse();

        std::cout << "[Parser AST] ";
        if (stmt) {
            stmt->accept(printer);
            std::cout << std::endl;
        } else {
            std::cout << "(null)" << std::endl;
            continue; // Skip to next SQL if parsing failed
        }

        std::cout << "[Semantic] Analyzing..." << std::endl;
        // run semantic analyzer; catch semantic errors so test continues
        try {
            sem.analyze(stmt.get());
            std::cout << "[Semantic] Analysis passed" << std::endl;
        }
        catch (const std::exception &e) {
            std::cerr << "[Semantic][ERROR] " << e.what() << std::endl;
            // continue to JSON output even if semantic check failed
        }

        std::cout << "[JSON] Serializing AST..." << std::endl;
        auto j = ASTJson::toJson(stmt.get());
        std::cout << j.dump(2) << std::endl;

        std::cout << "[Translate] Converting JSON to execution plan..." << std::endl;
        try {
            auto plan = JsonToPlan::translate(j);
            if (plan) {
                std::cout << "[Translate] Plan created successfully" << std::endl;
                
                std::cout << "[Execute] Running execution plan..." << std::endl;
                exec.execute(plan.get());
                std::cout << "[Execute] Execution completed successfully" << std::endl;
            } else {
                std::cerr << "[Translate] Failed to create execution plan" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "[Execute][ERROR] " << e.what() << std::endl;
        }
    }
        std::cout << "\n=== e2e flow test completed ===" << std::endl;
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
