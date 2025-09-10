#ifndef MINIBASE_AST_PRINTER_H
#define MINIBASE_AST_PRINTER_H

#include <string>
#include <sstream>
#include "ast.h"

// AST打印器类，用于可视化AST结构
class ASTPrinter : public ASTVisitor {
public:
    ASTPrinter() : indentLevel(0) {}
    
    // 获取打印结果
    std::string getResult() const { return output.str(); }
    
    // 访问者模式实现
    void visit(LiteralExpression& expr) override;
    void visit(IdentifierExpression& expr) override;
    void visit(BinaryExpression& expr) override;
    void visit(CreateTableStatement& stmt) override;
    void visit(InsertStatement& stmt) override;
    void visit(SelectStatement& stmt) override;
    void visit(DeleteStatement& stmt) override;
    
private:
    std::ostringstream output;
    int indentLevel;
    
    // 辅助方法
    void indent();
    void printIndent();
    std::string literalTypeToString(LiteralExpression::LiteralType type);
    std::string operatorToString(BinaryExpression::Operator op);
    std::string dataTypeToString(ColumnDefinition::DataType type);
};

#endif // MINIBASE_AST_PRINTER_H