#include "parser.h"

Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens), current(0) {}

Token Parser::peek() const {
    if (isAtEnd()) {
        return Token(TokenType::END_OF_FILE, "", 0, 0);
    }
    return tokens[current];
}

Token Parser::advance() {
    if (!isAtEnd()) {
        current++;
    }
    return tokens[current - 1];
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) {
        return false;
    }
    return peek().type == type;
}

Token Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) {
        return advance();
    }

    Token token = peek();
    throw ParseError(message, token.line, token.column);
}

bool Parser::isAtEnd() const {
    return current >= tokens.size() || tokens[current].type == TokenType::END_OF_FILE;
}

// 解析SQL语句
std::unique_ptr<Statement> Parser::parse() {
    try {
        auto stmt = statement();
        // 确保所有输入都被消费
        if (!isAtEnd()) {
            Token token = peek();
            throw ParseError("Unexpected token after statement", token.line, token.column);
        }
        return stmt;
    } catch (const ParseError& e) {
        // 重新抛出异常，让调用者处理
        throw;
    }
}

// 解析语句
std::unique_ptr<Statement> Parser::statement() {
    Token token = peek();
    
    if (token.type == TokenType::KEYWORD_CREATE) {
        return createStatement();
    } else if (token.type == TokenType::KEYWORD_INSERT) {
        return insertStatement();
    } else if (token.type == TokenType::KEYWORD_SELECT) {
        return selectStatement();
    } else if (token.type == TokenType::KEYWORD_DELETE) {
        return deleteStatement();
    }
    
    throw ParseError("Expected statement", token.line, token.column);
}

// 解析CREATE TABLE语句
std::unique_ptr<CreateTableStatement> Parser::createStatement() {
    // CREATE TABLE tableName ( columnDef, columnDef, ... )
    consume(TokenType::KEYWORD_CREATE, "Expected 'CREATE'");
    consume(TokenType::KEYWORD_TABLE, "Expected 'TABLE' after 'CREATE'");
    
    Token tableNameToken = consume(TokenType::IDENTIFIER, "Expected table name");
    std::string tableName = tableNameToken.lexeme;
    
    consume(TokenType::DELIMITER_LPAREN, "Expected '(' after table name");
    
    auto columns = columnDefinitions();
    
    consume(TokenType::DELIMITER_RPAREN, "Expected ')' after column definitions");
    consume(TokenType::DELIMITER_SEMICOLON, "Expected ';' after CREATE TABLE statement");
    
    return std::make_unique<CreateTableStatement>(tableName, std::move(columns));
}

// 解析列定义
std::vector<ColumnDefinition> Parser::columnDefinitions() {
    std::vector<ColumnDefinition> columns;
    
    do {
        if (check(TokenType::DELIMITER_RPAREN)) {
            break;
        }
        
        // columnName dataType
        Token nameToken = consume(TokenType::IDENTIFIER, "Expected column name");
        std::string columnName = nameToken.lexeme;
        
        ColumnDefinition::DataType dataType;
        if (match(TokenType::KEYWORD_INT)) {
            dataType = ColumnDefinition::DataType::INT;
        } else if (match(TokenType::KEYWORD_VARCHAR)) {
            dataType = ColumnDefinition::DataType::VARCHAR;
        } else {
            throw ParseError("Expected data type", peek().line, peek().column);
        }
        
        columns.emplace_back(columnName, dataType);
        
    } while (match(TokenType::DELIMITER_COMMA));
    
    return columns;
}

// 解析INSERT语句
std::unique_ptr<InsertStatement> Parser::insertStatement() {
    // INSERT INTO tableName (col1, col2, ...) VALUES (val1, val2, ...), (val1, val2, ...)
    consume(TokenType::KEYWORD_INSERT, "Expected 'INSERT'");
    consume(TokenType::KEYWORD_INTO, "Expected 'INTO' after 'INSERT'");
    
    Token tableNameToken = consume(TokenType::IDENTIFIER, "Expected table name");
    std::string tableName = tableNameToken.lexeme;
    
    // 解析列名列表
    consume(TokenType::DELIMITER_LPAREN, "Expected '(' after table name");
    auto columns = columnNames();
    consume(TokenType::DELIMITER_RPAREN, "Expected ')' after column names");
    
    // 解析VALUES子句
    consume(TokenType::KEYWORD_VALUES, "Expected 'VALUES'");
    auto values = valueLists();
    
    consume(TokenType::DELIMITER_SEMICOLON, "Expected ';' after INSERT statement");
    
    return std::make_unique<InsertStatement>(tableName, std::move(columns), std::move(values));
}

// 解析列名列表
std::vector<std::string> Parser::columnNames() {
    std::vector<std::string> columns;
    
    do {
        if (check(TokenType::DELIMITER_RPAREN)) {
            break;
        }
        
        Token nameToken = consume(TokenType::IDENTIFIER, "Expected column name");
        columns.push_back(nameToken.lexeme);
        
    } while (match(TokenType::DELIMITER_COMMA));
    
    return columns;
}

// 解析值列表
std::vector<ValueList> Parser::valueLists() {
    std::vector<ValueList> valueLists;
    
    do {
        consume(TokenType::DELIMITER_LPAREN, "Expected '('");
        
        std::vector<std::unique_ptr<Expression>> values;
        do {
            if (check(TokenType::DELIMITER_RPAREN)) {
                break;
            }
            
            values.push_back(primary());
            
        } while (match(TokenType::DELIMITER_COMMA));
        
        consume(TokenType::DELIMITER_RPAREN, "Expected ')'");
        
        valueLists.emplace_back(std::move(values));
        
    } while (match(TokenType::DELIMITER_COMMA));
    
    return valueLists;
}

// 解析SELECT语句
std::unique_ptr<SelectStatement> Parser::selectStatement() {
    // SELECT col1, col2, ... FROM tableName WHERE condition
    consume(TokenType::KEYWORD_SELECT, "Expected 'SELECT'");
    
    // 解析列名列表
    std::vector<std::string> columns;
    do {
        Token nameToken = consume(TokenType::IDENTIFIER, "Expected column name");
        columns.push_back(nameToken.lexeme);
    } while (match(TokenType::DELIMITER_COMMA));
    
    consume(TokenType::KEYWORD_FROM, "Expected 'FROM' after columns");
    
    Token tableNameToken = consume(TokenType::IDENTIFIER, "Expected table name");
    std::string tableName = tableNameToken.lexeme;
    
    // 可选的WHERE子句
    std::unique_ptr<Expression> whereClause = nullptr;
    if (match(TokenType::KEYWORD_WHERE)) {
        whereClause = expression();
    }
    
    consume(TokenType::DELIMITER_SEMICOLON, "Expected ';' after SELECT statement");
    
    return std::make_unique<SelectStatement>(std::move(columns), tableName, std::move(whereClause));
}

// 解析DELETE语句
std::unique_ptr<DeleteStatement> Parser::deleteStatement() {
    // DELETE FROM tableName WHERE condition
    consume(TokenType::KEYWORD_DELETE, "Expected 'DELETE'");
    consume(TokenType::KEYWORD_FROM, "Expected 'FROM' after 'DELETE'");
    
    Token tableNameToken = consume(TokenType::IDENTIFIER, "Expected table name");
    std::string tableName = tableNameToken.lexeme;
    
    // 可选的WHERE子句
    std::unique_ptr<Expression> whereClause = nullptr;
    if (match(TokenType::KEYWORD_WHERE)) {
        whereClause = expression();
    }
    
    consume(TokenType::DELIMITER_SEMICOLON, "Expected ';' after DELETE statement");
    
    return std::make_unique<DeleteStatement>(tableName, std::move(whereClause));
}

// 表达式解析
std::unique_ptr<Expression> Parser::expression() {
    return comparison();
}

std::unique_ptr<Expression> Parser::comparison() {
    auto expr = term();
    
    while (true) {
        BinaryExpression::Operator op;
        if (match(TokenType::OPERATOR_EQ)) {
            op = BinaryExpression::Operator::EQUALS;
        } else if (match(TokenType::OPERATOR_LT)) {
            op = BinaryExpression::Operator::LESS_THAN;
        } else if (match(TokenType::OPERATOR_GT)) {
            op = BinaryExpression::Operator::GREATER_THAN;
        } else if (match(TokenType::OPERATOR_LE)) {
            op = BinaryExpression::Operator::LESS_EQUAL;
        } else if (match(TokenType::OPERATOR_GE)) {
            op = BinaryExpression::Operator::GREATER_EQUAL;
        } else if (match(TokenType::OPERATOR_NE)) {
            op = BinaryExpression::Operator::NOT_EQUAL;
        } else {
            break;
        }
        
        auto right = term();
        expr = std::make_unique<BinaryExpression>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::term() {
    auto expr = factor();
    
    while (true) {
        BinaryExpression::Operator op;
        if (match(TokenType::OPERATOR_PLUS)) {
            op = BinaryExpression::Operator::PLUS;
        } else if (match(TokenType::OPERATOR_MINUS)) {
            op = BinaryExpression::Operator::MINUS;
        } else {
            break;
        }
        
        auto right = factor();
        expr = std::make_unique<BinaryExpression>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::factor() {
    auto expr = primary();
    
    while (true) {
        BinaryExpression::Operator op;
        if (match(TokenType::OPERATOR_TIMES)) {
            op = BinaryExpression::Operator::MULTIPLY;
        } else if (match(TokenType::OPERATOR_DIVIDE)) {
            op = BinaryExpression::Operator::DIVIDE;
        } else {
            break;
        }
        
        auto right = primary();
        expr = std::make_unique<BinaryExpression>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::primary() {
    if (match(TokenType::CONST_INT)) {
        Token token = tokens[current - 1];
        return std::make_unique<LiteralExpression>(LiteralExpression::LiteralType::INTEGER, token.lexeme);
    }
    
    if (match(TokenType::CONST_STRING)) {
        Token token = tokens[current - 1];
        return std::make_unique<LiteralExpression>(LiteralExpression::LiteralType::STRING, token.lexeme);
    }
    
    if (match(TokenType::IDENTIFIER)) {
        Token token = tokens[current - 1];
        return std::make_unique<IdentifierExpression>(token.lexeme);
    }
    
    if (match(TokenType::DELIMITER_LPAREN)) {
        auto expr = expression();
        consume(TokenType::DELIMITER_RPAREN, "Expected ')' after expression");
        return expr;
    }
    
    throw ParseError("Expected expression", peek().line, peek().column);
}