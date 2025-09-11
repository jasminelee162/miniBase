#pragma once

#include <string>

namespace SqlErrors {

// Lexer errors
inline constexpr const char* UNCLOSED_STRING = "Unclosed string literal";
inline constexpr const char* EXPECT_EQ_AFTER_BANG = "Expected '=' after '!'";
inline std::string unknownOperator(char op) { return std::string("Unknown operator: ") + op; }

// Parser generic
inline constexpr const char* UNEXPECTED_AFTER_STATEMENT = "Unexpected token after statement";
inline constexpr const char* EXPECT_STATEMENT = "Expected statement";
inline constexpr const char* EXPECT_CREATE = "Expected 'CREATE'";
inline constexpr const char* EXPECT_TABLE_AFTER_CREATE = "Expected 'TABLE' after 'CREATE'";
inline constexpr const char* EXPECT_TABLE_NAME = "Expected table name";
inline constexpr const char* EXPECT_LPAREN_AFTER_TABLE = "Expected '(' after table name";
inline constexpr const char* EXPECT_RPAREN_AFTER_COLUMNS = "Expected ')' after column definitions";
inline constexpr const char* EXPECT_SEMI_AFTER_CREATE = "Expected ';' after CREATE TABLE statement";
inline constexpr const char* EXPECT_COLUMN_NAME = "Expected column name";
inline constexpr const char* EXPECT_DATA_TYPE = "Expected data type";
inline constexpr const char* EXPECT_INSERT = "Expected 'INSERT'";
inline constexpr const char* EXPECT_INTO_AFTER_INSERT = "Expected 'INTO' after 'INSERT'";
inline constexpr const char* EXPECT_RPAREN_AFTER_COLS = "Expected ')' after column names";
inline constexpr const char* EXPECT_VALUES = "Expected 'VALUES'";
inline constexpr const char* EXPECT_SEMI_AFTER_INSERT = "Expected ';' after INSERT statement";
inline constexpr const char* EXPECT_LPAREN = "Expected '('";
inline constexpr const char* EXPECT_RPAREN = "Expected ')'";
inline constexpr const char* EXPECT_SELECT = "Expected 'SELECT'";
inline constexpr const char* EXPECT_FROM_AFTER_COLS = "Expected 'FROM' after columns";
inline constexpr const char* EXPECT_SEMI_AFTER_SELECT = "Expected ';' after SELECT statement";
inline constexpr const char* EXPECT_DELETE = "Expected 'DELETE'";
inline constexpr const char* EXPECT_FROM_AFTER_DELETE = "Expected 'FROM' after 'DELETE'";
inline constexpr const char* EXPECT_SEMI_AFTER_DELETE = "Expected ';' after DELETE statement";
inline constexpr const char* EXPECT_RPAREN_AFTER_EXPR = "Expected ')' after expression";
inline constexpr const char* EXPECT_EXPRESSION = "Expected expression";

// AST/JSON
inline constexpr const char* UNSUPPORTED_STMT_JSON = "Unsupported statement type for JSON serialization";

// Semantic analyzer
inline std::string noCurrentTableForIdentifier(const std::string& name) {
    return std::string("No current table context for identifier: ") + name;
}
inline std::string columnNotExist(const std::string& name) {
    return std::string("Column does not exist: ") + name;
}
inline std::string columnNotExistInTable(const std::string& col, const std::string& tbl) {
    return std::string("Column does not exist: ") + col + " in table " + tbl;
}
inline constexpr const char* TYPE_MISMATCH_BINARY = "Type mismatch in binary expression";
inline constexpr const char* UNKNOWN_EXPR_TYPE = "Unknown expression type";
inline std::string tableNotExist(const std::string& name) {
    return std::string("Table does not exist: ") + name;
}
inline std::string tableAlreadyExist(const std::string& name) {
    return std::string("Table already exists: ") + name;
}
inline std::string columnCountMismatch(size_t expected, size_t got) {
    return std::string("Column count mismatch: expected ") + std::to_string(expected) +
           ", got " + std::to_string(got);
}
inline constexpr const char* WHERE_MUST_BE_BOOL = "WHERE clause must evaluate to a boolean condition";

} // namespace SqlErrors 