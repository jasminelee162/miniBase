#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include "../lexer/lexer.h"
#include "../parser/parser.h"
#include "../parser/ast_printer.h"
#include "semantic_analyzer.h"

int main() {
    // 清空Catalog（用于测试）
    Catalog::getInstance().clear();
    
    // 测试SQL语句
    std::vector<std::string> testQueries = {
        // 正确的语句
        "CREATE TABLE student(id INT, name VARCHAR, age INT);",
        "INSERT INTO student(id,name,age) VALUES (1,'Alice',20);",
        "SELECT id,name FROM student WHERE age > 18;",
        "DELETE FROM student WHERE id = 1;",
        
        // 错误的语句
        "CREATE TABLE student(id INT, name VARCHAR, age INT);", // 表已存在
        "INSERT INTO nonexistent(id) VALUES (1);", // 表不存在
        "INSERT INTO student(nonexistent) VALUES (1);", // 列不存在
        "INSERT INTO student(id,name) VALUES (1,'Alice',20);", // 列数不匹配
        "INSERT INTO student(id,name,age) VALUES ('abc',123,20);", // 类型不匹配
        "SELECT nonexistent FROM student;", // 列不存在
        "DELETE FROM nonexistent WHERE id = 1;" // 表不存在
    };
    
    for (const auto& query : testQueries) {
        std::cout << "\nSQL: " << query << std::endl;
        std::cout << "-----------------------------------" << std::endl;
        
        try {
            // 词法分析
            Lexer lexer(query);
            std::vector<Token> tokens = lexer.tokenize();
            
            // 语法分析
            Parser parser(tokens);
            std::unique_ptr<Statement> ast = parser.parse();
            
            // 打印AST
            std::cout << "AST:" << std::endl;
            ASTPrinter printer;
            ast->accept(printer);
            std::cout << printer.getResult();
            
            // 语义分析
            std::cout << "\nSemantic Analysis:" << std::endl;
            SemanticAnalyzer analyzer;
            analyzer.analyze(ast.get());
            std::cout << "Semantic check passed!" << std::endl;
            
        } catch (const SemanticError& e) {
            std::cerr << "Semantic error: [" << SemanticError::errorTypeToString(e.getType()) << ", "
                      << e.getLine() << ", " << e.getColumn() << "] "
                      << e.what() << std::endl;
        } catch (const ParseError& e) {
            std::cerr << "Parse error at line " << e.getLine() << ", column " << e.getColumn()
                      << ": " << e.what() << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
        
        std::cout << "-----------------------------------" << std::endl;
    }
    
    return 0;
}