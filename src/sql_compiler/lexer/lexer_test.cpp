#include "lexer.h"
#include <iostream>
#include <string>
#include <vector>

// 辅助函数：将TokenType转换为字符串
std::string tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::KEYWORD_SELECT: return "KEYWORD_SELECT";
        case TokenType::KEYWORD_FROM: return "KEYWORD_FROM";
        case TokenType::KEYWORD_WHERE: return "KEYWORD_WHERE";
        case TokenType::KEYWORD_CREATE: return "KEYWORD_CREATE";
        case TokenType::KEYWORD_TABLE: return "KEYWORD_TABLE";
        case TokenType::KEYWORD_INSERT: return "KEYWORD_INSERT";
        case TokenType::KEYWORD_INTO: return "KEYWORD_INTO";
        case TokenType::KEYWORD_VALUES: return "KEYWORD_VALUES";
        case TokenType::KEYWORD_DELETE: return "KEYWORD_DELETE";
        case TokenType::KEYWORD_INT: return "KEYWORD_INT";
        case TokenType::KEYWORD_VARCHAR: return "KEYWORD_VARCHAR";
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::CONST_INT: return "CONST_INT";
        case TokenType::CONST_STRING: return "CONST_STRING";
        case TokenType::OPERATOR_EQ: return "OPERATOR_EQ";
        case TokenType::OPERATOR_LT: return "OPERATOR_LT";
        case TokenType::OPERATOR_GT: return "OPERATOR_GT";
        case TokenType::OPERATOR_LE: return "OPERATOR_LE";
        case TokenType::OPERATOR_GE: return "OPERATOR_GE";
        case TokenType::OPERATOR_NE: return "OPERATOR_NE";
        case TokenType::OPERATOR_PLUS: return "OPERATOR_PLUS";
        case TokenType::OPERATOR_MINUS: return "OPERATOR_MINUS";
        case TokenType::OPERATOR_TIMES: return "OPERATOR_TIMES";
        case TokenType::OPERATOR_DIVIDE: return "OPERATOR_DIVIDE";
        case TokenType::DELIMITER_SEMICOLON: return "DELIMITER_SEMICOLON";
        case TokenType::DELIMITER_COMMA: return "DELIMITER_COMMA";
        case TokenType::DELIMITER_LPAREN: return "DELIMITER_LPAREN";
        case TokenType::DELIMITER_RPAREN: return "DELIMITER_RPAREN";
        case TokenType::DELIMITER_DOT: return "DELIMITER_DOT";
        case TokenType::END_OF_FILE: return "END_OF_FILE";
        case TokenType::INVALID: return "INVALID";
        default: return "UNKNOWN";
    }
}

int main() {
    // 测试SQL语句
    std::vector<std::string> testQueries = {
        "CREATE TABLE student(id INT, name VARCHAR, age INT);",
        "INSERT INTO student(id,name,age) VALUES (1,'Alice',20);",
        "SELECT id,name FROM student WHERE age > 18;",
        "DELETE FROM student WHERE id = 1;",
        // 错误测试
        "SELECT * FROM student", // 缺少分号
        "INSERT INTO student(id,name,age) VALUES (1,'Alice,20);" // 未闭合的字符串
    };
    
    for (const auto& query : testQueries) {
        std::cout << "\nSQL: " << query << std::endl;
        std::cout << "Tokens:" << std::endl;
        
        Lexer lexer(query);
        std::vector<Token> tokens = lexer.tokenize();
        
        for (const auto& token : tokens) {
            std::cout << "[" << tokenTypeToString(token.type) << ", "
                      << token.lexeme << ", "
                      << token.line << ", "
                      << token.column << "]" << std::endl;
            
            // 如果遇到错误，停止处理当前查询
            if (token.type == TokenType::INVALID) {
                std::cout << "Error: " << token.lexeme << " at line " << token.line << ", column " << token.column << std::endl;
                break;
            }
        }
        
        std::cout << "-----------------------------------" << std::endl;
    }
    
    return 0;
}