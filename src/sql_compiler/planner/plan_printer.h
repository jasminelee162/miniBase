#ifndef MINIBASE_PLAN_PRINTER_H
#define MINIBASE_PLAN_PRINTER_H

#include <string>
#include <sstream>
#include "../../engine/operators/plan_node.h"

// 计划打印器类
class PlanPrinter {
public:
    PlanPrinter() : indentLevel(0) {}
    
    // 打印计划节点
    std::string print(const PlanNode* node) {
        result.str("");
        printNode(node);
        return result.str();
    }
    
private:
    std::stringstream result;
    int indentLevel;
    
    // 辅助方法
    void indent() { indentLevel++; }
    void dedent() { indentLevel--; }
    void printIndent();
    void printNode(const PlanNode* node);
    void printCreateTable(const PlanNode* node);
    void printInsert(const PlanNode* node);
    void printSeqScan(const PlanNode* node);
    void printProject(const PlanNode* node);
    void printFilter(const PlanNode* node);
    void printDelete(const PlanNode* node);
};

#endif // MINIBASE_PLAN_PRINTER_H