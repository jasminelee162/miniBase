#include "plan_printer.h"

void PlanPrinter::printIndent() {
    for (int i = 0; i < indentLevel; ++i) {
        result << "  ";
    }
}

void PlanPrinter::visit(CreateTablePlanNode& node) {
    printIndent();
    result << "CreateTable: " << node.getTableName() << std::endl;
    
    indent();
    printIndent();
    result << "Columns: [";
    const auto& columns = node.getColumns();
    for (size_t i = 0; i < columns.size(); ++i) {
        if (i > 0) result << ", ";
        result << columns[i].first << " " << ColumnInfo::dataTypeToString(columns[i].second);
    }
    result << "]" << std::endl;
    dedent();
}

void PlanPrinter::visit(InsertPlanNode& node) {
    printIndent();
    result << "Insert: " << node.getTableName() << std::endl;
    
    indent();
    
    // 打印列名
    printIndent();
    result << "Columns: [";
    const auto& columns = node.getColumnNames();
    for (size_t i = 0; i < columns.size(); ++i) {
        if (i > 0) result << ", ";
        result << columns[i];
    }
    result << "]" << std::endl;
    
    // 打印值列表
    printIndent();
    result << "Values: [" << std::endl;
    indent();
    const auto& valuesList = node.getValues();
    for (size_t i = 0; i < valuesList.size(); ++i) {
        printIndent();
        result << "(";
        for (size_t j = 0; j < valuesList[i].size(); ++j) {
            if (j > 0) result << ", ";
            result << valuesList[i][j].toString();
        }
        result << ")" << std::endl;
    }
    dedent();
    printIndent();
    result << "]" << std::endl;
    
    dedent();
}

void PlanPrinter::visit(SeqScanPlanNode& node) {
    printIndent();
    result << "SeqScan: " << node.getTableName() << std::endl;
    
    if (node.getPredicate()) {
        indent();
        printIndent();
        result << "Predicate: " << node.getPredicate()->toString() << std::endl;
        dedent();
    }
}

void PlanPrinter::visit(ProjectPlanNode& node) {
    printIndent();
    result << "Project: [";
    const auto& columns = node.getColumns();
    for (size_t i = 0; i < columns.size(); ++i) {
        if (i > 0) result << ", ";
        result << columns[i];
    }
    result << "]" << std::endl;
    
    indent();
    node.getChild()->accept(*this);
    dedent();
}

void PlanPrinter::visit(FilterPlanNode& node) {
    printIndent();
    result << "Filter: " << node.getPredicate()->toString() << std::endl;
    
    indent();
    node.getChild()->accept(*this);
    dedent();
}

void PlanPrinter::visit(DeletePlanNode& node) {
    printIndent();
    result << "Delete: " << node.getTableName() << std::endl;
    
    if (node.getPredicate()) {
        indent();
        printIndent();
        result << "Predicate: " << node.getPredicate()->toString() << std::endl;
        dedent();
    }
}