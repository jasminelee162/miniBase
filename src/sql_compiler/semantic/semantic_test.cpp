#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include "../lexer/lexer.h"
#include "../parser/parser.h"
#include "../parser/ast_printer.h"
#include "semantic_analyzer.h"

using namespace minidb;

int main() {
    std::vector<std::string> correct_sql_statements = {
        "CREATE TABLE students (id INT, name VARCHAR(20), age INT);",
        "INSERT INTO students VALUES (1, 'Alice', 20);",
        "SELECT id, name FROM students WHERE age > 18;",
        "DELETE FROM students WHERE id = 1;",
        "UPDATE teachers SET experience = 9 WHERE full_name = 'Bob Johnson';"
    };

    std::vector<std::string> error_sql_statements = {
        "CREATE TABLE students (id INT, name VARCHAR(20), age INT)", // Missing semicolon
        "INSERT INTO students VALUES (1, 'Alice', 20)", // Missing semicolon
        "SELECT id, name FROM students WHERE age > 18", // Missing semicolon
        "DELETE FROM students WHERE id = 1", // Missing semicolon
        "UPDATE teachers SET experience = 9 WHERE full_name = 'Bob Johnson'" // Missing semicolon
    };

    // Test correct SQL statements
    for (const auto& sql : correct_sql_statements) {
        std::cout << "\nTesting SQL: " << sql << std::endl;
        try {
            minidb::Catalog catalog;
            Lexer lexer(sql);
            std::vector<Token> tokens = lexer.tokenize();
            lexer.printTokens(tokens); // Print tokens for debugging
            Parser parser(tokens);
            auto ast = parser.parse();
            std::cout << "SQL parsed successfully." << std::endl;

            // Semantic analysis
            SemanticAnalyzer analyzer(&catalog);
            analyzer.analyze(ast.get());
            std::cout << "Semantic analysis successful." << std::endl;

            // Print AST (optional)
            // ASTPrinter printer;
            // printer.print(ast.get());

        } catch (const ParseError& e) {
            std::cerr << "Parse Error: " << e.what() << " at (" << e.getLine() << "," << e.getColumn() << ")" << std::endl;
        } catch (const SemanticError& e) {
            std::cerr << "Semantic Error: " << e.what() << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }

    // Test error SQL statements
    for (const auto& sql : error_sql_statements) {
        std::cout << "\nTesting error SQL: " << sql << std::endl;
        try {
            minidb::Catalog catalog;
            Lexer lexer(sql);
            std::vector<Token> tokens = lexer.tokenize();
            lexer.printTokens(tokens); // Print tokens for debugging
            Parser parser(tokens);
            auto ast = parser.parse();
            std::cerr << "Error: Expected parse error but got success for: " << sql << std::endl;
        } catch (const ParseError& e) {
            std::cout << "Caught expected parse error: " << e.what() << " at (" << e.getLine() << "," << e.getColumn() << ")" << std::endl;
        } catch (const SemanticError& e) {
            std::cerr << "Semantic Error: " << e.what() << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }

    return 0;
}