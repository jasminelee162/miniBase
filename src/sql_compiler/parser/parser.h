//parser.h
#ifndef MINIBASE_PARSER_H
#define MINIBASE_PARSER_H

#include <vector>
#include <memory>
#include <stdexcept>
#include <string>
#include "../lexer/token.h"
#include "ast.h"

// 解析错误异常
class ParseError : public std::runtime_error {
public:
    ParseError(const std::string& message, int line, int column)
        : std::runtime_error(message), line(line), column(column) {}

    int getLine() const { return line; }
    int getColumn() const { return column; }

private:
    int line;
    int column;
};

// SQL解析器类
class Parser {
public:
    Parser(const std::vector<Token>& tokens);

    // 解析SQL语句
    std::unique_ptr<Statement> parse();

private:
    const std::vector<Token>& tokens;
    size_t current;  // 当前Token索引

    // 辅助方法
    Token peek() const;
    Token advance();
    bool match(TokenType type);
    bool check(TokenType type) const;
    Token consume(TokenType type, const std::string& message);
    bool isAtEnd() const;

    // 递归下降解析方法
    std::unique_ptr<Statement> statement();
    std::unique_ptr<CreateTableStatement> createStatement();
    std::unique_ptr<InsertStatement> insertStatement();
    std::unique_ptr<SelectStatement> selectStatement();
    std::unique_ptr<DeleteStatement> deleteStatement();
    std::unique_ptr<UpdateStatement> updateStatement();
    // std::unique_ptr<JoinStatement> joinStatement();
    std::unique_ptr<ShowTablesStatement> showTablesStatement();
    std::unique_ptr<DropStatement> dropStatement();

    // 表达式解析
    std::unique_ptr<Expression> expression();
    std::unique_ptr<Expression> comparison();
    std::unique_ptr<Expression> term();
    std::unique_ptr<Expression> factor();
    std::unique_ptr<Expression> primary();

    // 辅助解析方法
    std::vector<ColumnDefinition> columnDefinitions();
    std::vector<std::string> columnNames();
    std::vector<ValueList> valueLists();

    std::string parseJoinCondition(); // 解析JOIN条件
    std::unique_ptr<JoinClause> parseJoinClause(); // 解析单个JOIN
    // bool isJoinStatement() const;
};

#endif // MINIBASE_PARSER_H