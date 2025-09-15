#include "lexer.h"
#include "../../util/logger.h"
#include "token.h"
#include <cctype>
#include <stdexcept>
#include "../common/error_messages.h"

Lexer::Lexer(const std::string& source)
    : input(source), position(0), line(1), column(1), currentChar('\0') {
    // 初始化关键字映射表（逐项赋值，兼容较旧的编译器）
    keywords["SELECT"] = TokenType::KEYWORD_SELECT;
    keywords["FROM"] = TokenType::KEYWORD_FROM;
    keywords["WHERE"] = TokenType::KEYWORD_WHERE;
    keywords["AS"] = TokenType::KEYWORD_AS;
    keywords["GROUP"] = TokenType::KEYWORD_GROUP_BY; 
    keywords["HAVING"] = TokenType::KEYWORD_HAVING;
    keywords["BY"] = TokenType::KEYWORD_BY;
    keywords["SUM"] = TokenType::KEYWORD_SUM;
    keywords["COUNT"] = TokenType::KEYWORD_COUNT;
    keywords["AVG"] = TokenType::KEYWORD_AVG;
    keywords["MIN"] = TokenType::KEYWORD_MIN;
    keywords["MAX"] = TokenType::KEYWORD_MAX;
    keywords["ORDER"] = TokenType::KEYWORD_ORDER;
    keywords["ASC"] = TokenType::KEYWORD_ASC;
    keywords["DESC"] = TokenType::KEYWORD_DESC;

    keywords["JOIN"] = TokenType::KEYWORD_JOIN;
    keywords["ON"] = TokenType::KEYWORD_ON;
    keywords["INNER"] = TokenType::KEYWORD_INNER;
    keywords["LEFT"] = TokenType::KEYWORD_LEFT;
    keywords["RIGHT"] = TokenType::KEYWORD_RIGHT;

    keywords["CREATE"] = TokenType::KEYWORD_CREATE;
    keywords["TABLE"] = TokenType::KEYWORD_TABLE;

    keywords["INSERT"] = TokenType::KEYWORD_INSERT;
    keywords["INTO"] = TokenType::KEYWORD_INTO;
    keywords["VALUES"] = TokenType::KEYWORD_VALUES;

    keywords["DELETE"] = TokenType::KEYWORD_DELETE;
    keywords["INT"] = TokenType::KEYWORD_INT;
    keywords["VARCHAR"] = TokenType::KEYWORD_VARCHAR;

    keywords["UPDATE"] = TokenType::KEYWORD_UPDATE;
    keywords["SET"] = TokenType::KEYWORD_SET;

    keywords["SHOW"] = TokenType::KEYWORD_SHOW;
    keywords["TABLES"] = TokenType::KEYWORD_TABLES;

    keywords["DROP"] = TokenType::KEYWORD_DROP;

    keywords["CALL"] = TokenType::KEYWORD_CALL;


    currentChar = input.empty() ? '\0' : input[0];
}

void Lexer::advance() {
    if (position < static_cast<int>(input.length())) {
        if (currentChar == '\n') {
            ++line;
            column = 0;
        }
        ++position;
        ++column;
    }
    currentChar = (position < static_cast<int>(input.length())) ? input[position] : '\0';
}

void Lexer::skipWhitespace() {
    while (currentChar != '\0' && std::isspace(static_cast<unsigned char>(currentChar))) {
        advance();
    }
}

char Lexer::peek() {
    return (position + 1 < static_cast<int>(input.length())) ? input[position + 1] : '\0';
}
//注释
void Lexer::skipLineComment() {
    // 验证当前位置确实是 "--" 的开始
    if (currentChar != '-' || peek() != '-') {
        return;
    }
    
    // 跳过 "--"
    advance(); // 第一个 '-'
    advance(); // 第二个 '-'
    
    // 跳过到行末或文件末尾
    while (currentChar != '\0' && currentChar != '\n' && currentChar != '\r') {
        advance();
    }
    
    // 处理换行符
    if (currentChar == '\r') {
        advance();
        if (currentChar == '\n') { // Windows风格 \r\n
            advance();
        }
    } else if (currentChar == '\n') { // Unix风格 \n
        advance();
    }
}

// 实现块注释跳过方法
bool Lexer::skipBlockComment() {
    // 验证当前位置确实是 "/*" 的开始
    if (currentChar != '/' || peek() != '*') {
        return false; // 不是块注释开始，返回 false
    }
    
    int startLine = line;
    int startColumn = column;
    
    // 跳过 "/*"
    advance(); // '/'
    advance(); // '*'
    
    // 查找结束标记 "*/"
    while (currentChar != '\0') {
        if (currentChar == '*' && peek() == '/') {
            // 找到结束标记
            advance(); // '*'
            advance(); // '/'
            return true; // 成功跳过块注释
        } else {
            advance();
        }
    }
    
    // 如果到达文件末尾仍未找到结束标记，抛出错误
    std::string errorMsg = "未关闭的块注释，开始于 (" + std::to_string(startLine) + 
                          "," + std::to_string(startColumn) + ")";
    throw std::runtime_error(errorMsg);
}

Token Lexer::scanIdentifier() {
    int startColumn = column;
    std::string result;

    while (currentChar != '\0' && (std::isalnum(static_cast<unsigned char>(currentChar)) || currentChar == '_')) {
        result += currentChar;
        advance();
    }

    std::string upperResult = result;
    for (char& c : upperResult) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

    auto it = keywords.find(upperResult);
    if (it != keywords.end()) {
        return Token(it->second, result, line, startColumn);
    }
    return Token(TokenType::IDENTIFIER, result, line, startColumn);
}

Token Lexer::scanNumber() {
    int startColumn = column;
    std::string result;
    while (currentChar != '\0' && std::isdigit(static_cast<unsigned char>(currentChar))) {
        result += currentChar;
        advance();
    }
    // detect invalid numeric literal like 123abc
    if (std::isalpha(static_cast<unsigned char>(currentChar)) || currentChar == '_') {
        std::string tail;
        while (currentChar != '\0' && (std::isalnum(static_cast<unsigned char>(currentChar)) || currentChar == '_')) {
            tail += currentChar;
            advance();
        }
        std::string full = result + tail;
        return Token(TokenType::INVALID, full, line, startColumn, std::string("Invalid numeric literal '") + full + "'");
    }
    return Token(TokenType::CONST_INT, result, line, startColumn);
}

Token Lexer::scanString() {
    int startColumn = column;
    std::string result;
    // 跳过开始的引号
    advance();
    while (currentChar != '\0' && currentChar != '\'') {
        if (currentChar == '\\' && peek() == '\'') {
            // consume the backslash, then take the escaped quote
            advance();
        }
        result += currentChar;
        advance();
    }
    if (currentChar == '\'') {
        advance();
        return Token(TokenType::CONST_STRING, result, line, startColumn);
    }
    return createErrorToken(SqlErrors::UNCLOSED_STRING, result);
}

Token Lexer::scanOperator() {
    int startColumn = column;
    char op = currentChar;
    TokenType type = TokenType::INVALID;

    switch (op) {
    case '=': type = TokenType::OPERATOR_EQ; break;
    case '<':
        advance();
        if (currentChar == '=') { advance(); return Token(TokenType::OPERATOR_LE, "<=", line, startColumn); }
        return Token(TokenType::OPERATOR_LT, "<", line, startColumn);
    case '>':
        advance();
        if (currentChar == '=') { advance(); return Token(TokenType::OPERATOR_GE, ">=", line, startColumn); }
        return Token(TokenType::OPERATOR_GT, ">", line, startColumn);
    case '!':
        advance();
        if (currentChar == '=') { advance(); return Token(TokenType::OPERATOR_NE, "!=", line, startColumn); }
        return createErrorToken(SqlErrors::EXPECT_EQ_AFTER_BANG, std::string(1, '!'));
    case '+': type = TokenType::OPERATOR_PLUS; break;
    case '-': type = TokenType::OPERATOR_MINUS; break;
    case '*': type = TokenType::OPERATOR_TIMES; break;
    case '/': type = TokenType::OPERATOR_DIVIDE; break;
    case ';': type = TokenType::DELIMITER_SEMICOLON; break;
    case ',': type = TokenType::DELIMITER_COMMA; break;
    case '(': type = TokenType::DELIMITER_LPAREN; break;
    case ')': type = TokenType::DELIMITER_RPAREN; break;
    case '.': type = TokenType::DELIMITER_DOT; break;
    default:
        return createErrorToken(SqlErrors::unknownOperator(op), std::string(1, op));
    }

    advance();
    return Token(type, std::string(1, op), line, startColumn);
}

Token Lexer::createErrorToken(const std::string& message, const std::string& lexeme) {
    return Token(TokenType::INVALID, lexeme, line, column, message);
}

Token Lexer::getNextToken() {
    while(true){
        skipWhitespace();
        if (currentChar == '\0') return Token(TokenType::END_OF_FILE, "", line, column);
        //检查注释
        if (currentChar == '-' && peek() == '-') {
            skipLineComment();
            continue; //跳过注释后继续循环
        }
        if (currentChar == '/' && peek() == '*') {
            try {
                skipBlockComment();
            } catch (const std::runtime_error& e) {
                return createErrorToken(e.what());
            }
            continue; //跳过注释后继续循环
        }
        break;
    }
    
    // 正常的 token 识别逻辑
        if (std::isalpha(static_cast<unsigned char>(currentChar)) || currentChar == '_') return scanIdentifier();
        if (std::isdigit(static_cast<unsigned char>(currentChar))) return scanNumber();
        if (currentChar == '\'') return scanString();
        return scanOperator();
    
}

std::vector<Token> Lexer::tokenize() {
    Logger logger("logs/lexer.log");
    logger.log("[Lexer] Start tokenizing input");
    std::vector<Token> tokens;
    Token token;
    do {
        token = getNextToken();
        // if the first token is a bare identifier (likely misspelled keyword), flag it
        if (tokens.empty() && token.type == TokenType::IDENTIFIER) {
            Token err(TokenType::INVALID, token.lexeme, token.line, token.column,
                      std::string("Unknown identifier '") + token.lexeme + "'");
            token = err;
        }
        // include error message if present
        std::string logLine = std::string("[Lexer] Token: ") + token.lexeme;
        if (!token.errorMessage.empty()) logLine += std::string(" ERROR: ") + token.errorMessage;
        logger.log(logLine);
        tokens.push_back(token);
    } while (token.type != TokenType::END_OF_FILE && token.type != TokenType::INVALID);
    logger.log("[Lexer] Tokenizing finished");
    return tokens;
}
