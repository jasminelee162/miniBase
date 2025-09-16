#include "plan_printer.h"

void PlanPrinter::printIndent() {
    for (int i = 0; i < indentLevel; ++i) {
        result << "  ";
    }
}

void PlanPrinter::printNode(const PlanNode* node) {
    if (!node) {
        printIndent();
        result << "NULL" << std::endl;
        return;
    }
    
    switch (node->type) {
        case PlanType::CreateTable:
            printCreateTable(node);
            break;
        case PlanType::Insert:
            printInsert(node);
            break;
        case PlanType::SeqScan:
            printSeqScan(node);
            break;
        case PlanType::Project:
            printProject(node);
            break;
        case PlanType::Filter:
            printFilter(node);
            break;
        case PlanType::Delete:
            printDelete(node);
            break;
        default:
            printIndent();
            result << "Unknown plan type" << std::endl;
            break;
    }
}

void PlanPrinter::printCreateTable(const PlanNode* node) {
    printIndent();
    result << "CreateTable: " << node->table_name << std::endl;
    
    indent();
    printIndent();
    result << "Columns: [";
    for (size_t i = 0; i < node->table_columns.size(); ++i) {  // ✅ 使用 table_columns.size()
        if (i > 0) result << ", ";
        const auto &c = node->table_columns[i];
        result << c.name << "(" << c.type;
        if (c.length > 0) {
            result << "(" << c.length << ")";  // ✅ 长度大于 0 才显示
        }
        result << ")";
        // 打印约束标记
        bool any = c.is_primary_key || c.is_unique || c.not_null || !c.default_value.empty();
        if (any) {
            result << " {";
            bool first = true;
            if (c.is_primary_key) { result << (first?"":"; ") << "PK"; first = false; }
            if (c.is_unique) { result << (first?"":"; ") << "UNIQUE"; first = false; }
            if (c.not_null) { result << (first?"":"; ") << "NOT NULL"; first = false; }
            if (!c.default_value.empty()) { result << (first?"":"; ") << "DEFAULT='" << c.default_value << "'"; }
            result << "}";
        }
    }
    result << "]" << std::endl;
    dedent();
}

void PlanPrinter::printInsert(const PlanNode* node) {
    printIndent();
    result << "Insert: " << node->table_name << std::endl;
    
    indent();
    
    // 打印列名
    printIndent();
    result << "Columns: [";
    for (size_t i = 0; i < node->columns.size(); ++i) {
        if (i > 0) result << ", ";
        result << node->columns[i];
    }
    result << "]" << std::endl;
    
    // 打印值列表
    printIndent();
    result << "Values: [" << std::endl;
    indent();
    for (size_t i = 0; i < node->values.size(); ++i) {
        printIndent();
        result << "(";
        for (size_t j = 0; j < node->values[i].size(); ++j) {
            if (j > 0) result << ", ";
            result << node->values[i][j];
        }
        result << ")" << std::endl;
    }
    dedent();
    printIndent();
    result << "]" << std::endl;
    
    dedent();
}

void PlanPrinter::printSeqScan(const PlanNode* node) {
    printIndent();
    result << "SeqScan: " << node->table_name << std::endl;
    
    if (!node->predicate.empty()) {
        indent();
        printIndent();
        result << "Predicate: " << node->predicate << std::endl;
        dedent();
    }
}

void PlanPrinter::printProject(const PlanNode* node) {
    printIndent();
    result << "Project: [";
    for (size_t i = 0; i < node->columns.size(); ++i) {
        if (i > 0) result << ", ";
        result << node->columns[i];
    }
    result << "]" << std::endl;
    
    indent();
    if (!node->children.empty()) {
        printNode(node->children[0].get());
    }
    dedent();
}

void PlanPrinter::printFilter(const PlanNode* node) {
    printIndent();
    result << "Filter: " << node->predicate << std::endl;
    
    indent();
    if (!node->children.empty()) {
        printNode(node->children[0].get());
    }
    dedent();
}

void PlanPrinter::printDelete(const PlanNode* node) {
    printIndent();
    result << "Delete: " << node->table_name << std::endl;
    
    if (!node->predicate.empty()) {
        indent();
        printIndent();
        result << "Predicate: " << node->predicate << std::endl;
        dedent();
    }
}
