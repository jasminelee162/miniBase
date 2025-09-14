#ifndef MINIBASE_SEMANTIC_ANALYZER_H
#define MINIBASE_SEMANTIC_ANALYZER_H

#include <string>
#include <vector>
#include <memory>
#include "../../catalog/catalog.h"
#include "../parser/ast.h"

using namespace minidb;

// 语义错误类
class SemanticError : public std::runtime_error {
public:
    enum class ErrorType {
        TABLE_NOT_EXIST,        // 表不存在
        TABLE_ALREADY_EXIST,   // 表已存在
        COLUMN_NOT_EXIST,      // 列不存在
        TYPE_MISMATCH,         // 类型不匹配
        COLUMN_COUNT_MISMATCH, // 列数不匹配
        UNKNOWN                // 未知错误
    };
    
    SemanticError(ErrorType type, const std::string& message, int line = 0, int column = 0)
        : std::runtime_error(message), type(type), line(line), column(column) {}
    
    ErrorType getType() const { return type; }
    int getLine() const { return line; }
    int getColumn() const { return column; }
    
    static std::string errorTypeToString(ErrorType type) {
        switch (type) {
            case ErrorType::TABLE_NOT_EXIST: return "TABLE_NOT_EXIST";
            case ErrorType::TABLE_ALREADY_EXIST: return "TABLE_ALREADY_EXIST";
            case ErrorType::COLUMN_NOT_EXIST: return "COLUMN_NOT_EXIST";
            case ErrorType::TYPE_MISMATCH: return "TYPE_MISMATCH";
            case ErrorType::COLUMN_COUNT_MISMATCH: return "COLUMN_COUNT_MISMATCH";
            case ErrorType::UNKNOWN: return "UNKNOWN";
            default: return "UNKNOWN";
        }
    }
    
private:
    ErrorType type;
    int line;
    int column;
};

// 语义分析器类
class SemanticAnalyzer : public ASTVisitor {
public:
    SemanticAnalyzer() : catalog_(nullptr) {}
    
    // 分析AST
    void analyze(Statement* stmt) {
        stmt->accept(*this);
    }
    
    // 设置Catalog指针
    void setCatalog(Catalog* catalog) {
        catalog_ = catalog;
    }
    
    // 访问者模式实现
    void visit(LiteralExpression& expr) override;
    void visit(IdentifierExpression& expr) override;
    void visit(BinaryExpression& expr) override;
    void visit(CreateTableStatement& stmt) override;
    void visit(InsertStatement& stmt) override;
    void visit(SelectStatement& stmt) override;
    void visit(DeleteStatement& stmt) override;
    void visit(UpdateStatement& stmt) override;
    
private:
    Catalog* catalog_;
    
    // 当前正在分析的表
    TableSchema currentTable_;
    
    // 辅助方法
    std::string getExpressionType(Expression* expr);
    void checkTableExists(const std::string& tableName);
    void checkTableNotExists(const std::string& tableName);
    void checkColumnExists(const std::string& tableName, const std::string& columnName);
    void checkTypeCompatibility(const std::string& expected, const std::string& actual, const std::string& context);
};

#endif // MINIBASE_SEMANTIC_ANALYZER_H