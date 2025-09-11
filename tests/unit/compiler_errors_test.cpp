#include "../../src/sql_compiler/lexer/lexer.h"
#include "../../src/sql_compiler/parser/parser.h"
#include "../../src/sql_compiler/semantic/semantic_analyzer.h"
#include "../../src/sql_compiler/common/error_messages.h"
#include <iostream>

int main() {
    int failures = 0;

    // 1) Lexer: unclosed string literal
    try {
        std::string sql = "SELECT 'abc"; // missing closing quote
        Lexer lx(sql);
        auto tokens = lx.tokenize();
        // tokenization should end with INVALID
        if (tokens.empty() || tokens.back().type != TokenType::INVALID) {
            std::cerr << "[Lexer] Expected INVALID token for unclosed string" << std::endl;
            failures++;
        } else if (tokens.back().errorMessage != SqlErrors::UNCLOSED_STRING) {
            std::cerr << "[Lexer] Unexpected message: " << tokens.back().errorMessage << std::endl;
            failures++;
        }
    } catch (const std::exception &e) {
        std::cerr << "[Lexer][EXCEPTION] " << e.what() << std::endl;
        failures++;
    }

    // 2) Parser: unexpected token after statement
    try {
        // two statements without supporting multi-stmt parsing
        std::string sql = "SELECT id FROM t; SELECT 1;";
        Lexer l(sql);
        auto toks = l.tokenize();
        Parser p(toks);
        (void)p.parse();
        std::cerr << "[Parser] Expected ParseError not thrown" << std::endl;
        failures++;
    } catch (const ParseError &e) {
        if (std::string(e.what()) != SqlErrors::UNEXPECTED_AFTER_STATEMENT) {
            std::cerr << "[Parser] Unexpected message: " << e.what() << std::endl;
            failures++;
        }
    } catch (const std::exception &e) {
        std::cerr << "[Parser][EXCEPTION] " << e.what() << std::endl;
        failures++;
    }

    // 3) Parser: missing FROM
    try {
        std::string sql = "SELECT id;";
        Lexer l(sql);
        auto toks = l.tokenize();
        Parser p(toks);
        (void)p.parse();
        std::cerr << "[Parser] Expected ParseError (missing FROM) not thrown" << std::endl;
        failures++;
    } catch (const ParseError &e) {
        if (std::string(e.what()) != SqlErrors::EXPECT_FROM_AFTER_COLS) {
            std::cerr << "[Parser] Unexpected message: " << e.what() << std::endl;
            failures++;
        }
    }

    // 4) Semantic: table not exist in SELECT
    try {
        std::string sql = "SELECT id FROM not_exist;";
        Lexer l(sql);
        auto toks = l.tokenize();
        Parser p(toks);
        auto stmt = p.parse();
        SemanticAnalyzer sem; // uses default catalog.dat
        sem.analyze(stmt.get());
        std::cerr << "[Semantic] Expected SemanticError not thrown" << std::endl;
        failures++;
    } catch (const SemanticError &e) {
        if (e.getType() != SemanticError::ErrorType::TABLE_NOT_EXIST) {
            std::cerr << "[Semantic] Unexpected type: " << SemanticError::errorTypeToString(e.getType()) << std::endl;
            failures++;
        }
    }

    // 5) Semantic: column not exist in SELECT
    try {
        std::string sql = "SELECT col_not_exist FROM users;"; // assumes users table may or may not exist
        Lexer l(sql);
        auto toks = l.tokenize();
        Parser p(toks);
        auto stmt = p.parse();
        SemanticAnalyzer sem;
        sem.analyze(stmt.get());
        std::cerr << "[Semantic] Expected SemanticError not thrown for column" << std::endl;
        failures++;
    } catch (const SemanticError &e) {
        if (e.getType() != SemanticError::ErrorType::TABLE_NOT_EXIST &&
            e.getType() != SemanticError::ErrorType::COLUMN_NOT_EXIST) {
            std::cerr << "[Semantic] Unexpected type: " << SemanticError::errorTypeToString(e.getType()) << std::endl;
            failures++;
        }
    }

    std::cout << (failures == 0 ? "compiler_errors_test passed" : "compiler_errors_test FAILED") << std::endl;
    return failures == 0 ? 0 : 1;
} 