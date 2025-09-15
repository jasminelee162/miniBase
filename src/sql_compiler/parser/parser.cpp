//parser.cpp
#include "parser.h"
#include "../../util/logger.h"
#include "../common/error_messages.h"

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
    Logger logger("logs/parser.log");
    logger.log("[Parser] Begin parse");
    try {
        auto stmt = statement();
        // 确保所有输入都被消费
        if (!isAtEnd()) {
            Token token = peek();
            throw ParseError(SqlErrors::UNEXPECTED_AFTER_STATEMENT, token.line, token.column);
        }
        logger.log("[Parser] Parse successful");
        return stmt;
    } catch (const ParseError& e) {
        logger.log(std::string("[Parser][ERROR] ") + e.what());
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
    else if (token.type == TokenType::KEYWORD_UPDATE) {
        return updateStatement();
    }
    else if (token.type == TokenType::KEYWORD_SHOW) {
        return showTablesStatement();
    }
    else if (token.type == TokenType::KEYWORD_DROP) {
        return dropStatement();
    }
    
    
    throw ParseError(SqlErrors::EXPECT_STATEMENT, token.line, token.column);
}

// 解析CREATE TABLE语句
std::unique_ptr<CreateTableStatement> Parser::createStatement() {
    // CREATE TABLE tableName ( columnDef, columnDef, ... )
    consume(TokenType::KEYWORD_CREATE, SqlErrors::EXPECT_CREATE);
    consume(TokenType::KEYWORD_TABLE, SqlErrors::EXPECT_TABLE_AFTER_CREATE);
    
    Token tableNameToken = consume(TokenType::IDENTIFIER, SqlErrors::EXPECT_TABLE_NAME);
    std::string tableName = tableNameToken.lexeme;
    
    consume(TokenType::DELIMITER_LPAREN, SqlErrors::EXPECT_LPAREN_AFTER_TABLE);
    
    auto columns = columnDefinitions();
    
    consume(TokenType::DELIMITER_RPAREN, SqlErrors::EXPECT_RPAREN_AFTER_COLUMNS);
    consume(TokenType::DELIMITER_SEMICOLON, SqlErrors::EXPECT_SEMI_AFTER_CREATE);
    
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
        Token nameToken = consume(TokenType::IDENTIFIER, SqlErrors::EXPECT_COLUMN_NAME);
        std::string columnName = nameToken.lexeme;
        
        ColumnDefinition::DataType dataType;
        if (match(TokenType::KEYWORD_INT)) {
            dataType = ColumnDefinition::DataType::INT;
        } else if (match(TokenType::KEYWORD_VARCHAR)) {
            dataType = ColumnDefinition::DataType::VARCHAR;
            // Optional: VARCHAR(length)
            if (match(TokenType::DELIMITER_LPAREN)) {
                // allow a simple integer literal as length
                if (match(TokenType::CONST_INT)) {
                    // ignore the actual length value; it's not stored in current schema
                } else {
                    Token t = peek();
                    throw ParseError(SqlErrors::EXPECT_VARCHAR_LENGTH, t.line, t.column);
                }
                consume(TokenType::DELIMITER_RPAREN, SqlErrors::EXPECT_RPAREN_AFTER_VARCHAR_LEN);
            }
        } else {
            throw ParseError(SqlErrors::EXPECT_DATA_TYPE, peek().line, peek().column);
        }
        
        columns.emplace_back(columnName, dataType);
        
    } while (match(TokenType::DELIMITER_COMMA));
    
    return columns;
}

// 解析INSERT语句
std::unique_ptr<InsertStatement> Parser::insertStatement() {
    // INSERT INTO tableName (col1, col2, ...) VALUES (val1, val2, ...), (val1, val2, ...)
    // 或 INSERT INTO tableName VALUES (val1, val2, ...), (val1, val2, ...)
    consume(TokenType::KEYWORD_INSERT, SqlErrors::EXPECT_INSERT);
    consume(TokenType::KEYWORD_INTO, SqlErrors::EXPECT_INTO_AFTER_INSERT);
    
    Token tableNameToken = consume(TokenType::IDENTIFIER, SqlErrors::EXPECT_TABLE_NAME);
    std::string tableName = tableNameToken.lexeme;
    
    // 解析列名列表（可选）
    std::vector<std::string> columns;
    if (check(TokenType::DELIMITER_LPAREN)) {
        consume(TokenType::DELIMITER_LPAREN, SqlErrors::EXPECT_LPAREN);
        columns = columnNames();
        consume(TokenType::DELIMITER_RPAREN, SqlErrors::EXPECT_RPAREN_AFTER_COLS);
    }
    // 如果没有列名列表，columns将保持为空，表示插入所有列
    
    // 解析VALUES子句
    consume(TokenType::KEYWORD_VALUES, SqlErrors::EXPECT_VALUES);
    auto values = valueLists();
    
    consume(TokenType::DELIMITER_SEMICOLON, SqlErrors::EXPECT_SEMI_AFTER_INSERT);
    
    return std::make_unique<InsertStatement>(tableName, std::move(columns), std::move(values));
}

// 解析列名列表
std::vector<std::string> Parser::columnNames() {
    std::vector<std::string> columns;
    
    do {
        if (check(TokenType::DELIMITER_RPAREN)) {
            break;
        }
        
        Token nameToken = consume(TokenType::IDENTIFIER, SqlErrors::EXPECT_COLUMN_NAME);
        columns.push_back(nameToken.lexeme);
        
    } while (match(TokenType::DELIMITER_COMMA));
    
    return columns;
}

// 解析值列表
std::vector<ValueList> Parser::valueLists() {
    std::vector<ValueList> valueLists;
    
    do {
        consume(TokenType::DELIMITER_LPAREN, SqlErrors::EXPECT_LPAREN);
        
        std::vector<std::unique_ptr<Expression>> values;
        do {
            if (check(TokenType::DELIMITER_RPAREN)) {
                break;
            }
            
            values.push_back(primary());
            
        } while (match(TokenType::DELIMITER_COMMA));
        
        consume(TokenType::DELIMITER_RPAREN, SqlErrors::EXPECT_RPAREN);
        
        valueLists.emplace_back(std::move(values));
        
    } while (match(TokenType::DELIMITER_COMMA));
    
    return valueLists;
}

// 解析SELECT语句
std::unique_ptr<SelectStatement> Parser::selectStatement() {
    // SELECT col1, col2, ... FROM tableName WHERE condition
    consume(TokenType::KEYWORD_SELECT, SqlErrors::EXPECT_SELECT);
    
    // 解析列名列表
    std::vector<std::string> columns;
        //聚合函数
    std::vector<std::unique_ptr<AggregateExpression>> aggregates;

    //解析列表过程
    if (match(TokenType::OPERATOR_TIMES)) {
        columns.push_back("*");
    } else {
        // 需要至少一个列名
        if (!check(TokenType::IDENTIFIER)) {
            Token t = peek();
            throw ParseError("Expected identifier after SELECT", t.line, t.column);
        }
        do {
            //检查是否是聚合函数
            if (check(TokenType::KEYWORD_SUM) || check(TokenType::KEYWORD_COUNT) || 
                check(TokenType::KEYWORD_AVG) || check(TokenType::KEYWORD_MIN) || 
                check(TokenType::KEYWORD_MAX)) {
                
                Token funcToken = advance(); // 获取函数名
                consume(TokenType::DELIMITER_LPAREN, SqlErrors::EXPECT_LPAREN);
                Token colToken = consume(TokenType::IDENTIFIER, SqlErrors::EXPECT_COLUMN_NAME);
                consume(TokenType::DELIMITER_RPAREN, SqlErrors::EXPECT_RPAREN);
                
                std::string alias;
                if (match(TokenType::KEYWORD_AS)) {
                    Token aliasToken = consume(TokenType::IDENTIFIER, SqlErrors::EXPECT_IDENTIFIER);
                    alias = aliasToken.lexeme;
                }
                
                aggregates.push_back(std::make_unique<AggregateExpression>(
                    funcToken.lexeme, colToken.lexeme, alias));
            } else {
                // 普通列名
            Token nameToken = consume(TokenType::IDENTIFIER, SqlErrors::EXPECT_COLUMN_NAME);
            columns.push_back(nameToken.lexeme);
            }
        } while (match(TokenType::DELIMITER_COMMA));
    }
    
    if (!check(TokenType::KEYWORD_FROM)) {
        Token t = peek();
        throw ParseError("缺少'FROM' 在 '" + t.lexeme + "'之前", t.line, t.column);
    }
    consume(TokenType::KEYWORD_FROM, SqlErrors::EXPECT_FROM_AFTER_COLS);
    
    if (!check(TokenType::IDENTIFIER)) {
        Token t = peek();
        throw ParseError("FROM之后要有表名", t.line, t.column);
    }
    Token mainTableNameToken = consume(TokenType::IDENTIFIER, SqlErrors::EXPECT_TABLE_NAME);
    std::string mainTableName = mainTableNameToken.lexeme; //mainTableName=tableName

    // 可选的WHERE子句
    std::unique_ptr<Expression> whereClause = nullptr;
    if (match(TokenType::KEYWORD_WHERE)) {
        whereClause = expression();
    }
    
    // 可选的GROUP BY子句
       std::vector<std::string> groupByColumns;
    if (match(TokenType::KEYWORD_GROUP_BY)) {
        consume(TokenType::KEYWORD_BY, SqlErrors::EXPECT_BY_AFTER_GROUP);
        do {
            Token colToken = consume(TokenType::IDENTIFIER, SqlErrors::EXPECT_COLUMN_NAME);
            groupByColumns.push_back(colToken.lexeme);
        } while (match(TokenType::DELIMITER_COMMA));
    }
    
        // HAVING 子句
        std::unique_ptr<Expression> havingClause = nullptr;
        if (match(TokenType::KEYWORD_HAVING)) {
            havingClause = expression();
        }
    
    // 可选的 ORDER BY 子句
    std::vector<std::string> orderByColumns;
    bool orderByDesc = false;
    
    if (check(TokenType::KEYWORD_ORDER)) {
        advance(); // consume 'ORDER'
        consume(TokenType::KEYWORD_BY, SqlErrors::EXPECT_BY_AFTER_ORDER);
        
        // 解析排序列（支持多列，但先实现单列）
        do {
            Token colToken = consume(TokenType::IDENTIFIER, SqlErrors::EXPECT_COLUMN_NAME);
            orderByColumns.push_back(colToken.lexeme);
        } while (match(TokenType::DELIMITER_COMMA));
        
        // 检查 ASC/DESC（可选，默认 ASC）
        if (match(TokenType::KEYWORD_ASC)) {
            orderByDesc = false;
        } else if (match(TokenType::KEYWORD_DESC)) {
            orderByDesc = true;
        }
        // 如果都没有，保持默认的 orderByDesc = false (ASC)
    }
    //可选的join子句
    // ===== 解析 JOIN 子句 =====
    std::vector<JoinClause> joins;
    
    while (check(TokenType::KEYWORD_JOIN) || check(TokenType::KEYWORD_INNER) || 
           check(TokenType::KEYWORD_LEFT) || check(TokenType::KEYWORD_RIGHT)) {
        
        std::string joinType = "INNER"; // 默认
        
        // 解析 JOIN 类型
        if (match(TokenType::KEYWORD_INNER)) {
            joinType = "INNER";
            consume(TokenType::KEYWORD_JOIN, SqlErrors::EXPECT_JOIN_AFTER_TYPE);
        } else if (match(TokenType::KEYWORD_LEFT)) {
            joinType = "LEFT";
            consume(TokenType::KEYWORD_JOIN, SqlErrors::EXPECT_JOIN_AFTER_TYPE);
        } else if (match(TokenType::KEYWORD_RIGHT)) {
            joinType = "RIGHT";
            consume(TokenType::KEYWORD_JOIN, SqlErrors::EXPECT_JOIN_AFTER_TYPE);
        } else if (match(TokenType::KEYWORD_JOIN)) {
            joinType = "INNER";
        }

        // 解析连接的表名
        Token joinTableToken = consume(TokenType::IDENTIFIER, SqlErrors::EXPECT_TABLE_NAME);
        std::string joinTableName = joinTableToken.lexeme;

        // 解析 ON 条件
        consume(TokenType::KEYWORD_ON, SqlErrors::EXPECT_ON_AFTER_JOIN);
        std::string joinCondition = parseJoinCondition();

        // 添加到 JOIN 列表
        joins.emplace_back(joinType, joinTableName, joinCondition);
    }

    /* -------在上方增加功能----------*/
    consume(TokenType::DELIMITER_SEMICOLON, SqlErrors::EXPECT_SEMI_AFTER_SELECT);
    return std::make_unique<SelectStatement>(
        std::move(columns), 
        std::move(aggregates), 
        mainTableName, 
        std::move(joins),
        std::move(whereClause), 
        std::move(groupByColumns), 
        std::move(havingClause),
        std::move(orderByColumns), 
        orderByDesc);
}
std::string Parser::parseJoinCondition() {
    // 解析 table1.column = table2.column 格式
    Token leftTable = consume(TokenType::IDENTIFIER, SqlErrors::EXPECT_TABLE_NAME);
    consume(TokenType::DELIMITER_DOT, SqlErrors::EXPECT_DOT);
    Token leftCol = consume(TokenType::IDENTIFIER, SqlErrors::EXPECT_COLUMN_NAME);
    
    consume(TokenType::OPERATOR_EQ, SqlErrors::EXPECT_EQUALS);
    
    Token rightTable = consume(TokenType::IDENTIFIER, SqlErrors::EXPECT_TABLE_NAME);
    consume(TokenType::DELIMITER_DOT, SqlErrors::EXPECT_DOT);
    Token rightCol = consume(TokenType::IDENTIFIER, SqlErrors::EXPECT_COLUMN_NAME);

    return leftTable.lexeme + "." + leftCol.lexeme + "=" + 
           rightTable.lexeme + "." + rightCol.lexeme;
}

// 解析DELETE语句
std::unique_ptr<DeleteStatement> Parser::deleteStatement() {
    // DELETE FROM tableName WHERE condition
    consume(TokenType::KEYWORD_DELETE, SqlErrors::EXPECT_DELETE);
    consume(TokenType::KEYWORD_FROM, SqlErrors::EXPECT_FROM_AFTER_DELETE);
    
    Token tableNameToken = consume(TokenType::IDENTIFIER, SqlErrors::EXPECT_TABLE_NAME);
    std::string tableName = tableNameToken.lexeme;
    
    // 可选的WHERE子句
    std::unique_ptr<Expression> whereClause = nullptr;
    if (match(TokenType::KEYWORD_WHERE)) {
        whereClause = expression();
    }
    
    consume(TokenType::DELIMITER_SEMICOLON, SqlErrors::EXPECT_SEMI_AFTER_DELETE);
    
    return std::make_unique<DeleteStatement>(tableName, std::move(whereClause));
}

//解析update语句
std::unique_ptr<UpdateStatement> Parser::updateStatement() {
    // UPDATE tableName SET col1 = val1, col2 = val2 WHERE condition
    consume(TokenType::KEYWORD_UPDATE, SqlErrors::EXPECT_UPDATE);
    
    Token tableNameToken = consume(TokenType::IDENTIFIER, SqlErrors::EXPECT_TABLE_NAME);
    std::string tableName = tableNameToken.lexeme;
    
    consume(TokenType::KEYWORD_SET, SqlErrors::EXPECT_SET_AFTER_UPDATE);
    
    std::vector<std::pair<std::string, std::unique_ptr<Expression>>> assignments;
    do {
        Token columnToken = consume(TokenType::IDENTIFIER, SqlErrors::EXPECT_COLUMN_NAME);
        std::string columnName = columnToken.lexeme;
        
        consume(TokenType::OPERATOR_EQ, SqlErrors::EXPECT_EQUALS_IN_ASSIGNMENT);
        
        auto valueExpr = expression();
        
        assignments.emplace_back(columnName, std::move(valueExpr));
        
    } while (match(TokenType::DELIMITER_COMMA));
    
    // 可选的WHERE子句
    std::unique_ptr<Expression> whereClause = nullptr;
    if (match(TokenType::KEYWORD_WHERE)) {
        whereClause = expression();
    }
    
    consume(TokenType::DELIMITER_SEMICOLON, SqlErrors::EXPECT_SEMI_AFTER_UPDATE);
    
    return std::make_unique<UpdateStatement>(tableName, std::move(assignments), std::move(whereClause));

}

//解析Show语句
std::unique_ptr<ShowTablesStatement> Parser::showTablesStatement(){
        consume(TokenType::KEYWORD_SHOW, SqlErrors::EXPECT_SHOW);
        consume(TokenType::KEYWORD_TABLES, SqlErrors::EXPECT_TABLES_AFTER_SHOW);
        consume(TokenType::DELIMITER_SEMICOLON, SqlErrors::EXPECT_SEMI_AFTER_SHOW_TABLES);
        
        return std::make_unique<ShowTablesStatement>();
}

//解析Drop语句
std::unique_ptr<DropStatement> Parser::dropStatement(){
        consume(TokenType::KEYWORD_DROP, SqlErrors::EXPECT_DROP);
        consume(TokenType::KEYWORD_TABLE, SqlErrors::EXPECT_TABLE_AFTER_DROP);
        
        Token tableNameToken = consume(TokenType::IDENTIFIER, SqlErrors::EXPECT_TABLE_NAME);
        std::string tableName = tableNameToken.lexeme;
        
        consume(TokenType::DELIMITER_SEMICOLON, SqlErrors::EXPECT_SEMI_AFTER_DROP);
        
        return std::make_unique<DropStatement>(tableName);
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
        consume(TokenType::DELIMITER_RPAREN, SqlErrors::EXPECT_RPAREN_AFTER_EXPR);
        return expr;
    }
    
    throw ParseError(SqlErrors::EXPECT_EXPRESSION, peek().line, peek().column);
}

