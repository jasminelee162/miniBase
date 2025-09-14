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
    skipWhitespace();
    if (currentChar == '\0') return Token(TokenType::END_OF_FILE, "", line, column);
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
