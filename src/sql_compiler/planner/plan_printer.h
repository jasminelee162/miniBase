#ifndef MINIBASE_PLAN_PRINTER_H
#define MINIBASE_PLAN_PRINTER_H

#include <string>
#include <sstream>
#include "../../engine/operators/plan_node.h"

// 计划打印器类
class PlanPrinter : public PlanVisitor {
public:
    PlanPrinter() : indentLevel(0) {}
    
    // 获取打印结果
    std::string getResult() const {
        return result.str();
    }
    
    // 访问者模式实现
    void visit(CreateTablePlanNode& node) override;
    void visit(InsertPlanNode& node) override;
    void visit(SeqScanPlanNode& node) override;
    void visit(ProjectPlanNode& node) override;
    void visit(FilterPlanNode& node) override;
    void visit(DeletePlanNode& node) override;
    
private:
    std::stringstream result;
    int indentLevel;
    
    // 辅助方法
    void indent() { indentLevel++; }
    void dedent() { indentLevel--; }
    void printIndent();
};

#endif // MINIBASE_PLAN_PRINTER_H