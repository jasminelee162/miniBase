#include "ast.h"

// 实现AST节点的accept方法

void LiteralExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void IdentifierExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void BinaryExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void CreateTableStatement::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void InsertStatement::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void SelectStatement::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void DeleteStatement::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}