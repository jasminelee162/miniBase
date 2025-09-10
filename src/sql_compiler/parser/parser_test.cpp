#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include "../lexer/lexer.h"
#include "parser.h"
#include "ast_printer.h"

int main() {
    // 测试SQL语句
    std::vector<std::string> testQueries = {
        "CREATE TABLE student(id INT, name VARCHAR, age INT);",
        "INSERT INTO student(id,name,age) VALUES (1,'Alice',20);",
        "SELECT id,name FROM student WHERE age > 18;",
        "DELETE FROM student WHERE id = 1;"
    };
    
    for (const auto& query : testQueries) {
        std::cout << "\nSQL: " << query << std::endl;
        std::cout << "-----------------------------------" << std::endl;
        
        try {
            // 词法分析
            Lexer lexer(query);
            std::vector<Token> tokens = lexer.tokenize();
            
            std::cout << "Tokens:" << std::endl;
            for (const auto& token : tokens) {
                if (token.type == TokenType::END_OF_FILE) {
                    break;
                }
                std::cout << "[" << token.lexeme << "]";
            }
            std::cout << std::endl;
            
            // 语法分析
            Parser parser(tokens);
            std::unique_ptr<Statement> ast = parser.parse();
            
            // 打印AST
            std::cout << "\nAST:" << std::endl;
            ASTPrinter printer;
            ast->accept(printer);
            std::cout << printer.getResult();
            
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