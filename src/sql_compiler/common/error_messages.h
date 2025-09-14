#pragma once

#include <string>

namespace SqlErrors {

// Lexer errors
inline constexpr const char* UNCLOSED_STRING = "字符串字面量未闭合";
inline constexpr const char* EXPECT_EQ_AFTER_BANG = "在 '!' 之后缺少 '='";
inline std::string unknownOperator(char op) { return std::string("未知运算符: ") + op; }

// Parser generic
inline constexpr const char* UNEXPECTED_AFTER_STATEMENT = "在语句结束后存在多余的内容";
inline constexpr const char* EXPECT_STATEMENT = "缺少有效的语句";
inline constexpr const char* EXPECT_CREATE = "缺少关键字 'CREATE'";
inline constexpr const char* EXPECT_TABLE_AFTER_CREATE = "在 'CREATE' 之后缺少关键字 'TABLE'";
inline constexpr const char* EXPECT_TABLE_NAME = "缺少表名";
inline constexpr const char* EXPECT_LPAREN_AFTER_TABLE = "在表名之后缺少 '('";
inline constexpr const char* EXPECT_RPAREN_AFTER_COLUMNS = "在列定义之后缺少 ')'";
inline constexpr const char* EXPECT_SEMI_AFTER_CREATE = "在 CREATE TABLE 语句末尾缺少 ';'";
inline constexpr const char* EXPECT_COLUMN_NAME = "缺少列名";
inline constexpr const char* EXPECT_DATA_TYPE = "缺少数据类型";
inline constexpr const char* EXPECT_INSERT = "缺少关键字 'INSERT'";
inline constexpr const char* EXPECT_INTO_AFTER_INSERT = "在 'INSERT' 之后缺少关键字 'INTO'";
inline constexpr const char* EXPECT_RPAREN_AFTER_COLS = "在列名列表之后缺少 ')'";
inline constexpr const char* EXPECT_VALUES = "缺少关键字 'VALUES'";
inline constexpr const char* EXPECT_SEMI_AFTER_INSERT = "在 INSERT 语句末尾缺少 ';'";
inline constexpr const char* EXPECT_LPAREN = "缺少 '('";
inline constexpr const char* EXPECT_RPAREN = "缺少 ')'";
inline constexpr const char* EXPECT_SELECT = "缺少关键字 'SELECT'";
inline constexpr const char* EXPECT_FROM_AFTER_COLS = "在列名之后缺少关键字 'FROM'";
inline constexpr const char* EXPECT_SEMI_AFTER_SELECT = "在 SELECT 语句末尾缺少 ';'";
inline constexpr const char* EXPECT_DELETE = "缺少关键字 'DELETE'";
inline constexpr const char* EXPECT_FROM_AFTER_DELETE = "在 'DELETE' 之后缺少关键字 'FROM'";
inline constexpr const char* EXPECT_SEMI_AFTER_DELETE = "在 DELETE 语句末尾缺少 ';'";
inline constexpr const char* EXPECT_RPAREN_AFTER_EXPR = "在表达式之后缺少 ')'";
inline constexpr const char* EXPECT_UPDATE = "缺少关键字 UPDATE";
inline constexpr const char* EXPECT_SET_AFTER_UPDATE = "在 UPDATE 之后缺少 SET";
inline constexpr const char* EXPECT_EQUALS_IN_ASSIGNMENT = "赋值语句缺少等号 '='";
inline constexpr const char* EXPECT_VALUE_IN_ASSIGNMENT = "赋值语句缺少值";
inline constexpr const char* EXPECT_SEMI_AFTER_UPDATE = "UPDATE 语句末尾缺少分号 ';'";
inline constexpr const char* EXPECT_EXPR_AFTER_WHERE = "WHERE 子句缺少条件表达式";
inline constexpr const char* EXPECT_EXPRESSION = "缺少表达式";
// VARCHAR length specific
inline constexpr const char* EXPECT_VARCHAR_LENGTH = "在 'VARCHAR(' 之后期望长度";
inline constexpr const char* EXPECT_RPAREN_AFTER_VARCHAR_LEN = "在 VARCHAR 长度之后期望出现 ')'";

// AST/JSON
inline constexpr const char* UNSUPPORTED_STMT_JSON = "该语句类型暂不支持生成 JSON";

// Semantic analyzer
inline std::string noCurrentTableForIdentifier(const std::string& name) {
    return std::string("无当前表上下文，无法解析标识符: ") + name;
}
inline std::string columnNotExist(const std::string& name) {
    return std::string("列不存在: ") + name;
}
inline std::string columnNotExistInTable(const std::string& col, const std::string& tbl) {
    return std::string("列不存在: ") + col + "，所在表: " + tbl;
}
inline constexpr const char* TYPE_MISMATCH_BINARY = "二元表达式类型不匹配";
inline constexpr const char* UNKNOWN_EXPR_TYPE = "未知的表达式类型";
inline std::string tableNotExist(const std::string& name) {
    return std::string("表不存在: ") + name;
}
inline std::string tableAlreadyExist(const std::string& name) {
    return std::string("表已存在: ") + name;
}
inline std::string columnCountMismatch(size_t expected, size_t got) {
    return std::string("列数不匹配: 期望 ") + std::to_string(expected) +
           "，实际 " + std::to_string(got);
}
inline constexpr const char* WHERE_MUST_BE_BOOL = "WHERE 子句必须为布尔条件";

} // namespace SqlErrors