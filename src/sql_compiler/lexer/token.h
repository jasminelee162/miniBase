#ifndef MINIBASE_TOKEN_H
#define MINIBASE_TOKEN_H

#include <string>

// Token类型枚举
enum class TokenType {
    // 关键字
    //Select
    KEYWORD_SELECT,
    KEYWORD_FROM,
    KEYWORD_AS,
    KEYWORD_WHERE,

    KEYWORD_GROUP_BY, //聚合
    KEYWORD_BY,        // 需要单独的 BY token
    KEYWORD_HAVING,
    // 聚合函数
    KEYWORD_SUM,
    KEYWORD_COUNT,
    KEYWORD_AVG,
    KEYWORD_MIN,
    KEYWORD_MAX,
    
    //ORDER BY
    KEYWORD_ORDER,
    KEYWORD_ASC,
    KEYWORD_DESC,  

    //Join
    KEYWORD_JOIN,
    KEYWORD_ON,
    KEYWORD_INNER,
    KEYWORD_LEFT,
    KEYWORD_RIGHT,
      
    //CREATE
    KEYWORD_CREATE,
    KEYWORD_TABLE,
    //INSERT
    KEYWORD_INSERT,
    KEYWORD_INTO,
    KEYWORD_VALUES,
    //DELETE
    KEYWORD_DELETE,

    //UPDATE
    KEYWORD_UPDATE,
    KEYWORD_SET,

    //Show
    KEYWORD_SHOW,
    KEYWORD_TABLES,

    //DORP
    KEYWORD_DROP,

    // PROCEDURE
    KEYWORD_CALL,

    //数据类型
    KEYWORD_INT,
    KEYWORD_VARCHAR,
    
    // 标识符
    IDENTIFIER,
    
    // 常量
    CONST_INT,
    CONST_STRING,
    
    // 运算符
    OPERATOR_EQ,     // =
    OPERATOR_LT,     // <
    OPERATOR_GT,     // >
    OPERATOR_LE,     // <=
    OPERATOR_GE,     // >=
    OPERATOR_NE,     // !=
    OPERATOR_PLUS,   // +
    OPERATOR_MINUS,  // -
    OPERATOR_TIMES,  // *
    OPERATOR_DIVIDE, // /
    
    // 分隔符
    DELIMITER_SEMICOLON, // ;
    DELIMITER_COMMA,     // ,
    DELIMITER_LPAREN,    // (
    DELIMITER_RPAREN,    // )
    DELIMITER_DOT,       // .
    
    // 其他
    END_OF_FILE,
    INVALID
};

// Token结构体
struct Token {
    TokenType type;     // 种别码
    std::string lexeme; // 词素值
    int line;           // 行号
    int column;         // 列号
    std::string errorMessage; // 错误信息
    
    Token(TokenType t, const std::string& l, int ln, int col)
        : type(t), lexeme(l), line(ln), column(col), errorMessage("") {}
    
    // 带错误信息的构造函数
    Token(TokenType t, const std::string& l, int ln, int col, const std::string& err)
        : type(t), lexeme(l), line(ln), column(col), errorMessage(err) {}
    
    // 默认构造函数
    Token() : type(TokenType::INVALID), lexeme(""), line(0), column(0), errorMessage("") {}
};

#endif // MINIBASE_TOKEN_H