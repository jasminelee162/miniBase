#ifndef MINIBASE_LEXER_H
#define MINIBASE_LEXER_H

#include <string>
#include <vector>
#include <unordered_map>
#include "token.h"

class Lexer {
private:
    std::string input;          // 输入的SQL语句
    int position;               // 当前位置
    int line;                   // 当前行号
    int column;                 // 当前列号
    char currentChar;           // 当前字符

    // 关键字映射表
    std::unordered_map<std::string, TokenType> keywords;

    // 辅助方法
    void advance();             // 前进一个字符
    void skipWhitespace();      // 跳过空白字符
    char peek();                // 查看下一个字符但不前进

    // Token识别方法
    Token scanIdentifier();     // 扫描标识符或关键字
    Token scanNumber();         // 扫描数字
    Token scanString();         // 扫描字符串
    Token scanOperator();       // 扫描运算符

    // 错误处理
    Token createErrorToken(const std::string& message);

public:
    Lexer(const std::string& source);

    // 获取下一个Token
    Token getNextToken(); //语法分析器调用

    // 将整个输入转换为Token流
    std::vector<Token> tokenize(); //用于调试测试
};

#endif // MINIBASE_LEXER_H