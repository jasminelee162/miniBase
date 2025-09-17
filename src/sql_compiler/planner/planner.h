#ifndef MINIBASE_PLANNER_H
#define MINIBASE_PLANNER_H

#include <memory>
#include <stdexcept>
#include <string>
#include "../parser/ast.h"
#include "../../engine/operators/plan_node.h"
#include "../../catalog/catalog.h"

// 计划生成错误类
class PlannerError : public std::runtime_error {
public:
    enum class ErrorType {
        UNSUPPORTED_FEATURE,  // 不支持的功能
        MISSING_SEMANTIC_INFO, // 缺少语义信息
        UNKNOWN              // 未知错误
    };
    
    PlannerError(ErrorType type, const std::string& message)
        : std::runtime_error(message), type(type) {}
    
    ErrorType getType() const { return type; }
    
    static std::string errorTypeToString(ErrorType type) {
        switch (type) {
            case ErrorType::UNSUPPORTED_FEATURE: return "UNSUPPORTED_FEATURE";
            case ErrorType::MISSING_SEMANTIC_INFO: return "MISSING_SEMANTIC_INFO";
            case ErrorType::UNKNOWN: return "UNKNOWN";
            default: return "UNKNOWN";
        }
    }
    
private:
    ErrorType type;
};

// 计划生成器类
class Planner : public ASTVisitor {
public:
    Planner() {}
    
    // 生成执行计划
    std::unique_ptr<PlanNode> plan(Statement* stmt) {
        currentPlan = nullptr;
        stmt->accept(*this);
        return std::move(currentPlan);
    }
    
    // 访问者模式实现
    void visit(LiteralExpression& expr) override;
    void visit(IdentifierExpression& expr) override;
    void visit(BinaryExpression& expr) override;
    void visit(AggregateExpression& expr) override;
    void visit(SubqueryExpression& expr) override;
    void visit(CreateTableStatement& stmt) override;
    void visit(InsertStatement& stmt) override;
    void visit(SelectStatement& stmt) override;
    void visit(DeleteStatement& stmt) override;
    void visit(UpdateStatement& stmt) override;
    void visit(ShowTablesStatement& stmt) override;
    void visit(DropStatement& stmt) override;
    void visit(CallProcedureStatement& stmt) override;
    void visit(CreateProcedureStatement& stmt) override;
    void visit(CreateIndexStatement& stmt) override;
    
private:
    std::unique_ptr<PlanNode> currentPlan;
    
    // 辅助方法
    std::string expressionToString(Expression* expr);
    std::string expressionToPredicate(Expression* expr);
};


#endif // MINIBASE_PLANNER_H