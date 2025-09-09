#include "semantic_analyzer.h"

// 获取表达式的数据类型
ColumnInfo::DataType SemanticAnalyzer::getExpressionType(Expression* expr) {
    if (auto literal = dynamic_cast<LiteralExpression*>(expr)) {
        if (literal->getType() == LiteralExpression::LiteralType::INTEGER) {
            return ColumnInfo::DataType::INT;
        } else if (literal->getType() == LiteralExpression::LiteralType::STRING) {
            return ColumnInfo::DataType::VARCHAR;
        }
    } else if (auto identifier = dynamic_cast<IdentifierExpression*>(expr)) {
        if (!currentTable) {
            throw SemanticError(SemanticError::ErrorType::UNKNOWN, 
                              "No current table context for identifier: " + identifier->getName());
        }
        
        const ColumnInfo* column = currentTable->getColumn(identifier->getName());
        if (!column) {
            throw SemanticError(SemanticError::ErrorType::COLUMN_NOT_EXIST, 
                              "Column does not exist: " + identifier->getName());
        }
        
        return column->getType();
    } else if (auto binary = dynamic_cast<BinaryExpression*>(expr)) {
        ColumnInfo::DataType leftType = getExpressionType(binary->getLeft());
        ColumnInfo::DataType rightType = getExpressionType(binary->getRight());
        
        // 简单类型检查：对于比较操作，返回INT类型
        BinaryExpression::Operator op = binary->getOperator();
        if (op == BinaryExpression::Operator::EQUALS ||
            op == BinaryExpression::Operator::NOT_EQUAL ||
            op == BinaryExpression::Operator::LESS_THAN ||
            op == BinaryExpression::Operator::GREATER_THAN ||
            op == BinaryExpression::Operator::LESS_EQUAL ||
            op == BinaryExpression::Operator::GREATER_EQUAL) {
            return ColumnInfo::DataType::INT;
        }
        
        // 对于算术操作，如果两边都是INT，则结果是INT
        if (leftType == ColumnInfo::DataType::INT && rightType == ColumnInfo::DataType::INT) {
            return ColumnInfo::DataType::INT;
        }
        
        // 其他情况抛出类型不匹配错误
        throw SemanticError(SemanticError::ErrorType::TYPE_MISMATCH, 
                          "Type mismatch in binary expression");
    }
    
    throw SemanticError(SemanticError::ErrorType::UNKNOWN, "Unknown expression type");
}

// 检查表是否存在
void SemanticAnalyzer::checkTableExists(const std::string& tableName) {
    if (!catalog.hasTable(tableName)) {
        throw SemanticError(SemanticError::ErrorType::TABLE_NOT_EXIST, 
                          "Table does not exist: " + tableName);
    }
}

// 检查表是否不存在
void SemanticAnalyzer::checkTableNotExists(const std::string& tableName) {
    if (catalog.hasTable(tableName)) {
        throw SemanticError(SemanticError::ErrorType::TABLE_ALREADY_EXIST, 
                          "Table already exists: " + tableName);
    }
}

// 检查列是否存在
void SemanticAnalyzer::checkColumnExists(const std::string& tableName, const std::string& columnName) {
    auto table = catalog.getTable(tableName);
    if (!table) {
        throw SemanticError(SemanticError::ErrorType::TABLE_NOT_EXIST, 
                          "Table does not exist: " + tableName);
    }
    
    if (!table->hasColumn(columnName)) {
        throw SemanticError(SemanticError::ErrorType::COLUMN_NOT_EXIST, 
                          "Column does not exist: " + columnName + " in table " + tableName);
    }
}

// 检查类型兼容性
void SemanticAnalyzer::checkTypeCompatibility(ColumnInfo::DataType expected, ColumnInfo::DataType actual, 
                                           const std::string& context) {
    if (expected != actual) {
        throw SemanticError(SemanticError::ErrorType::TYPE_MISMATCH, 
                          "Type mismatch in " + context + ": expected " + 
                          ColumnInfo::dataTypeToString(expected) + ", got " + 
                          ColumnInfo::dataTypeToString(actual));
    }
}

// 访问者模式实现
void SemanticAnalyzer::visit(LiteralExpression& expr) {
    // 字面量表达式不需要特殊的语义检查
}

void SemanticAnalyzer::visit(IdentifierExpression& expr) {
    // 标识符表达式在getExpressionType中已经进行了检查
}

void SemanticAnalyzer::visit(BinaryExpression& expr) {
    // 递归检查左右子表达式
    expr.getLeft()->accept(*this);
    expr.getRight()->accept(*this);
    
    // 类型检查在getExpressionType中已经进行
}

void SemanticAnalyzer::visit(CreateTableStatement& stmt) {
    // 检查表是否已存在
    checkTableNotExists(stmt.getTableName());
    
    // 准备列定义
    std::vector<std::pair<std::string, ColumnInfo::DataType>> columns;
    for (const auto& col : stmt.getColumns()) {
        ColumnInfo::DataType type;
        if (col.getType() == ColumnDefinition::DataType::INT) {
            type = ColumnInfo::DataType::INT;
        } else if (col.getType() == ColumnDefinition::DataType::VARCHAR) {
            type = ColumnInfo::DataType::VARCHAR;
        } else {
            throw SemanticError(SemanticError::ErrorType::UNKNOWN, 
                              "Unknown column type for column: " + col.getName());
        }
        
        columns.emplace_back(col.getName(), type);
    }
    
    // 创建表
    catalog.createTable(stmt.getTableName(), columns);
}

void SemanticAnalyzer::visit(InsertStatement& stmt) {
    // 检查表是否存在
    checkTableExists(stmt.getTableName());
    auto table = catalog.getTable(stmt.getTableName());
    currentTable = table;
    
    // 检查列名是否存在
    for (const auto& colName : stmt.getColumnNames()) {
        checkColumnExists(stmt.getTableName(), colName);
    }
    
    // 检查列数是否匹配
    size_t expectedColumnCount = stmt.getColumnNames().size();
    for (const auto& valueList : stmt.getValueLists()) {
        if (valueList.getValues().size() != expectedColumnCount) {
            throw SemanticError(SemanticError::ErrorType::COLUMN_COUNT_MISMATCH, 
                              "Column count mismatch: expected " + std::to_string(expectedColumnCount) + 
                              ", got " + std::to_string(valueList.getValues().size()));
        }
        
        // 检查每个值的类型是否与列类型匹配
        for (size_t i = 0; i < expectedColumnCount; ++i) {
            const std::string& colName = stmt.getColumnNames()[i];
            const ColumnInfo* colInfo = table->getColumn(colName);
            Expression* valueExpr = valueList.getValues()[i].get();
            
            // 递归检查表达式
            valueExpr->accept(*this);
            
            // 检查类型兼容性
            ColumnInfo::DataType valueType = getExpressionType(valueExpr);
            checkTypeCompatibility(colInfo->getType(), valueType, 
                                 "INSERT value for column " + colName);
        }
    }
    
    currentTable = nullptr;
}

void SemanticAnalyzer::visit(SelectStatement& stmt) {
    // 检查表是否存在
    checkTableExists(stmt.getTableName());
    auto table = catalog.getTable(stmt.getTableName());
    currentTable = table;
    
    // 检查列名是否存在
    for (const auto& colName : stmt.getColumns()) {
        checkColumnExists(stmt.getTableName(), colName);
    }
    
    // 检查WHERE子句
    if (stmt.getWhereClause()) {
        stmt.getWhereClause()->accept(*this);
        
        // WHERE子句的结果应该是布尔类型（在我们的简化模型中用INT表示）
        ColumnInfo::DataType whereType = getExpressionType(stmt.getWhereClause());
        if (whereType != ColumnInfo::DataType::INT) {
            throw SemanticError(SemanticError::ErrorType::TYPE_MISMATCH, 
                              "WHERE clause must evaluate to a boolean condition");
        }
    }
    
    currentTable = nullptr;
}

void SemanticAnalyzer::visit(DeleteStatement& stmt) {
    // 检查表是否存在
    checkTableExists(stmt.getTableName());
    auto table = catalog.getTable(stmt.getTableName());
    currentTable = table;
    
    // 检查WHERE子句
    if (stmt.getWhereClause()) {
        stmt.getWhereClause()->accept(*this);
        
        // WHERE子句的结果应该是布尔类型（在我们的简化模型中用INT表示）
        ColumnInfo::DataType whereType = getExpressionType(stmt.getWhereClause());
        if (whereType != ColumnInfo::DataType::INT) {
            throw SemanticError(SemanticError::ErrorType::TYPE_MISMATCH, 
                              "WHERE clause must evaluate to a boolean condition");
        }
    }
    
    currentTable = nullptr;
}