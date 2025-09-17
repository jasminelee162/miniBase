#include "ast_printer.h"

void ASTPrinter::indent() {
    indentLevel += 2;
}

void ASTPrinter::printIndent() {
    for (int i = 0; i < indentLevel; i++) {
        output << " ";
    }
}

std::string ASTPrinter::literalTypeToString(LiteralExpression::LiteralType type) {
    switch (type) {
        case LiteralExpression::LiteralType::INTEGER: return "INTEGER";
        case LiteralExpression::LiteralType::STRING: return "STRING";
        default: return "UNKNOWN";
    }
}

std::string ASTPrinter::operatorToString(BinaryExpression::Operator op) {
    switch (op) {
        case BinaryExpression::Operator::EQUALS: return "=";
        case BinaryExpression::Operator::LESS_THAN: return "<";
        case BinaryExpression::Operator::GREATER_THAN: return ">";
        case BinaryExpression::Operator::LESS_EQUAL: return "<=";
        case BinaryExpression::Operator::GREATER_EQUAL: return ">=";
        case BinaryExpression::Operator::NOT_EQUAL: return "!=";
        case BinaryExpression::Operator::PLUS: return "+";
        case BinaryExpression::Operator::MINUS: return "-";
        case BinaryExpression::Operator::MULTIPLY: return "*";
        case BinaryExpression::Operator::DIVIDE: return "/";
        default: return "UNKNOWN";
    }
}

std::string ASTPrinter::dataTypeToString(ColumnDefinition::DataType type) {
    switch (type) {
        case ColumnDefinition::DataType::INT: return "INT";
        case ColumnDefinition::DataType::VARCHAR: return "VARCHAR";
        default: return "UNKNOWN";
    }
}

void ASTPrinter::visit(LiteralExpression& expr) {
    printIndent();
    output << "Literal(" << literalTypeToString(expr.getType()) << "): "
           << expr.getValue() << "\n";
}

void ASTPrinter::visit(IdentifierExpression& expr) {
    printIndent();
    output << "Identifier: " << expr.getName() << "\n";
}

void ASTPrinter::visit(BinaryExpression& expr) {
    printIndent();
    output << "BinaryExpression: " << operatorToString(expr.getOperator()) << "\n";
    
    indent();
    printIndent();
    output << "Left:\n";
    indent();
    expr.getLeft()->accept(*this);
    indentLevel -= 2;
    
    printIndent();
    output << "Right:\n";
    indent();
    expr.getRight()->accept(*this);
    indentLevel -= 2;
    
    indentLevel -= 2;
}

void ASTPrinter::visit(AggregateExpression& expr) {
    printIndent();
    output << "AggregateExpression(" << expr.getFunction() << "): " 
           << expr.getColumn();
    if (!expr.getAlias().empty()) {
        output << " AS " << expr.getAlias();
    }
    output << "\n";
}

void ASTPrinter::visit(SubqueryExpression& expr) {
    printIndent();
    output << "SubqueryExpression:\n";
    indent();
    expr.getSubquery()->accept(*this);
    indentLevel -= 2;
}

void ASTPrinter::visit(CreateTableStatement& stmt) {
    printIndent();
    output << "CreateTableStatement:\n";
    indent();
    
    printIndent();
    output << "Table: " << stmt.getTableName() << "\n";
    
    printIndent();
    output << "Columns:\n";
    indent();
    
    for (const auto& column : stmt.getColumns()) {
        printIndent();
        output << column.getName() << ": " << dataTypeToString(column.getType()) << "\n";
    }
    
    indentLevel -= 2;
    indentLevel -= 2;
}

void ASTPrinter::visit(InsertStatement& stmt) {
    printIndent();
    output << "InsertStatement:\n";
    indent();
    
    printIndent();
    output << "Table: " << stmt.getTableName() << "\n";
    
    printIndent();
    output << "Columns:\n";
    indent();
    for (const auto& column : stmt.getColumnNames()) {
        printIndent();
        output << column << "\n";
    }
    indentLevel -= 2;
    
    printIndent();
    output << "Values:\n";
    indent();
    
    int rowIndex = 0;
    for (const auto& valueList : stmt.getValueLists()) {
        printIndent();
        output << "Row " << rowIndex++ << ":\n";
        indent();
        
        int valueIndex = 0;
        for (const auto& value : valueList.getValues()) {
            printIndent();
            output << "Value " << valueIndex++ << ":\n";
            indent();
            value->accept(*this);
            indentLevel -= 2;
        }
        
        indentLevel -= 2;
    }
    
    indentLevel -= 2;
    indentLevel -= 2;
}

void ASTPrinter::visit(SelectStatement& stmt) {
    printIndent();
    output << "SelectStatement:\n";
    indent();
    
    printIndent();
    output << "Table: " << stmt.getTableName() << "\n";
    
    printIndent();
    output << "Columns:\n";
    indent();
    for (const auto& column : stmt.getColumns()) {
        printIndent();
        output << column << "\n";
    }
    indentLevel -= 2;
    
    if (stmt.getWhereClause()) {
        printIndent();
        output << "Where:\n";
        indent();
        stmt.getWhereClause()->accept(*this);
        indentLevel -= 2;
    }
    
    indentLevel -= 2;
}

void ASTPrinter::visit(DeleteStatement& stmt) {
    printIndent();
    output << "DeleteStatement:\n";
    indent();
    
    printIndent();
    output << "Table: " << stmt.getTableName() << "\n";
    
    if (stmt.getWhereClause()) {
        printIndent();
        output << "Where:\n";
        indent();
        stmt.getWhereClause()->accept(*this);
        indentLevel -= 2;
    }
    
    indentLevel -= 2;
}

void ASTPrinter::visit(UpdateStatement& stmt) {
    printIndent();
    output << "UpdateStatement:\n";
    indent();
    
    printIndent();
    output << "Table: " << stmt.getTableName() << "\n";
    
    if (stmt.getWhereClause()) {
        printIndent();
        output << "Where:\n";
        indent();
        stmt.getWhereClause()->accept(*this);
        indentLevel -= 2;
    }
    
    indentLevel -= 2;
}

void ASTPrinter::visit(ShowTablesStatement& stmt) {
    printIndent();
    output << "ShowTablesStatement\n";
}

void ASTPrinter::visit(DropStatement& stmt) {
    printIndent();
    output << "DropStatement:\n";
    indent();
    
    printIndent();
    output << "Table: " << stmt.getTableName() << "\n";
    
    indentLevel -= 2;
}

void ASTPrinter::visit(CallProcedureStatement& stmt) {
    printIndent();
    output << "CallProcedureStatement:\n";
    indent();
    
    printIndent();
    output << "Procedure: " << stmt.getProcName() << "\n";
    
    if (!stmt.getArgs().empty()) {
        printIndent();
        output << "Arguments: ";
        for (size_t i = 0; i < stmt.getArgs().size(); ++i) {
            if (i > 0) output << ", ";
            output << stmt.getArgs()[i];
        }
        output << "\n";
    }
    
    indentLevel -= 2;
}

void ASTPrinter::visit(CreateProcedureStatement& stmt) {
    printIndent();
    output << "CreateProcedureStatement:\n";
    indent();
    
    printIndent();
    output << "Procedure: " << stmt.getProcName() << "\n";
    
    if (!stmt.getParams().empty()) {
        printIndent();
        output << "Parameters: ";
        for (size_t i = 0; i < stmt.getParams().size(); ++i) {
            if (i > 0) output << ", ";
            output << stmt.getParams()[i];
        }
        output << "\n";
    }
    
    printIndent();
    output << "Body: " << stmt.getBody() << "\n";
    
    indentLevel -= 2;
}

void ASTPrinter::visit(CreateIndexStatement& stmt) {
    printIndent();
    output << "CreateIndexStatement:\n";
    indent();
    
    printIndent();
    output << "Index: " << stmt.getIndexName() << "\n";
    printIndent();
    output << "Table: " << stmt.getTableName() << "\n";
    printIndent();
    output << "Type: " << stmt.getIndexType() << "\n";
    
    if (!stmt.getColumns().empty()) {
        printIndent();
        output << "Columns: ";
        for (size_t i = 0; i < stmt.getColumns().size(); ++i) {
            if (i > 0) output << ", ";
            output << stmt.getColumns()[i];
        }
        output << "\n";
    }
    
    indentLevel -= 2;
}