#include "lexer.h"
#include "token.h"
#include <cctype>
#include <stdexcept>

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
    return Token(TokenType::INVALID, result, line, startColumn, "Unclosed string literal");
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
        return Token(TokenType::INVALID, "!", line, startColumn, "Expected '=' after '!'");
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
        return Token(TokenType::INVALID, std::string(1, op), line, startColumn, std::string("Unknown operator: ") + op);
    }

    advance();
    return Token(type, std::string(1, op), line, startColumn);
}

Token Lexer::createErrorToken(const std::string& message) {
    return Token(TokenType::INVALID, "", line, column, message);
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
    std::vector<Token> tokens;
    Token token;
    do {
        token = getNextToken();
        tokens.push_back(token);
    } while (token.type != TokenType::END_OF_FILE && token.type != TokenType::INVALID);
    return tokens;
}