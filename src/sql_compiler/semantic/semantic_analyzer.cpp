#include "semantic_analyzer.h"
#include "../../catalog/catalog.h"
#include "../../util/logger.h"
#include "../common/error_messages.h"

// 获取表达式的数据类型
std::string SemanticAnalyzer::getExpressionType(Expression *expr)
{
    Logger logger("logs/semantic.log");
    if (auto literal = dynamic_cast<LiteralExpression *>(expr))
    {
        if (literal->getType() == LiteralExpression::LiteralType::INTEGER)
        {
            return "INT";
        }
        else if (literal->getType() == LiteralExpression::LiteralType::STRING)
        {
            // 空字符串代表解析阶段的 NULL
            if (literal->getValue().empty()) return "NULL";
            return "VARCHAR";
        }
    }
    else if (auto identifier = dynamic_cast<IdentifierExpression *>(expr))
    {
        // 检查当前表是否有效
        if (!currentTable_ || currentTable_->table_name.empty())
        {
            throw SemanticError(SemanticError::ErrorType::UNKNOWN,
                                SqlErrors::noCurrentTableForIdentifier(identifier->getName()));
        }

        // 查找列
        bool found = false;
        std::string columnType;
        for (const auto &col : currentTable_->columns)
        {
            if (col.name == identifier->getName())
            {
                found = true;
                columnType = col.type;
                break;
            }
        }

        if (!found)
        {
            throw SemanticError(SemanticError::ErrorType::COLUMN_NOT_EXIST,
                                SqlErrors::columnNotExist(identifier->getName()));
        }

        return columnType;
    }
    else if (auto binary = dynamic_cast<BinaryExpression *>(expr))
    {
        std::string leftType = getExpressionType(binary->getLeft());
        std::string rightType = getExpressionType(binary->getRight());

        // 简单类型检查：对于比较操作，返回INT类型
        BinaryExpression::Operator op = binary->getOperator();
        if (op == BinaryExpression::Operator::EQUALS ||
            op == BinaryExpression::Operator::NOT_EQUAL ||
            op == BinaryExpression::Operator::LESS_THAN ||
            op == BinaryExpression::Operator::GREATER_THAN ||
            op == BinaryExpression::Operator::LESS_EQUAL ||
            op == BinaryExpression::Operator::GREATER_EQUAL)
        {
            return "INT";
        }

        // 对于算术操作，如果两边都是INT，则结果是INT
        if (leftType == "INT" && rightType == "INT")
        {
            return "INT";
        }

        // 其他情况抛出类型不匹配错误
        throw SemanticError(SemanticError::ErrorType::TYPE_MISMATCH,
                            SqlErrors::TYPE_MISMATCH_BINARY);
    }

    throw SemanticError(SemanticError::ErrorType::UNKNOWN, SqlErrors::UNKNOWN_EXPR_TYPE);
}

// 检查表是否存在
void SemanticAnalyzer::checkTableExists(const std::string &tableName)
{
    Logger logger("logs/semantic.log");
    logger.log(std::string("[Semantic] checkTableExists: ") + tableName);
    if (!catalog_ || !catalog_->HasTable(tableName))
    {
        logger.log(std::string("[Semantic][ERROR] table not found: ") + tableName);
        throw SemanticError(SemanticError::ErrorType::TABLE_NOT_EXIST,
                            SqlErrors::tableNotExist(tableName));
    }
}

// 检查表是否不存在
void SemanticAnalyzer::checkTableNotExists(const std::string &tableName)
{
    Logger logger("logs/semantic.log");
    logger.log(std::string("[Semantic] checkTableNotExists: ") + tableName);
    if (catalog_ && catalog_->HasTable(tableName))
    {
        logger.log(std::string("[Semantic][ERROR] table already exists: ") + tableName);
        throw SemanticError(SemanticError::ErrorType::TABLE_ALREADY_EXIST,
                            SqlErrors::tableAlreadyExist(tableName));
    }
}

// 检查列是否存在
void SemanticAnalyzer::checkColumnExists(const std::string &tableName, const std::string &columnName)
{
    if (!catalog_ || !catalog_->HasTable(tableName))
    {
        throw SemanticError(SemanticError::ErrorType::TABLE_NOT_EXIST,
                            SqlErrors::tableNotExist(tableName));
    }

    auto table = catalog_->GetTable(tableName);
    bool found = false;
    for (const auto &col : table.columns)
    {
        if (col.name == columnName)
        {
            found = true;
            break;
        }
    }

    if (!found)
    {
        throw SemanticError(SemanticError::ErrorType::COLUMN_NOT_EXIST,
                            SqlErrors::columnNotExistInTable(columnName, tableName));
    }
}

// 检查类型兼容性
void SemanticAnalyzer::checkTypeCompatibility(const std::string &expected, const std::string &actual,
                                              const std::string &context)
{
    if (actual == "NULL") return; // 允许 NULL 与任意类型兼容（后续由执行阶段处理 DEFAULT/NOT NULL）
    if (expected != actual)
    {
        throw SemanticError(SemanticError::ErrorType::TYPE_MISMATCH,
                            std::string("Type mismatch in ") + context + ": expected " +
                                expected + ", got " + actual);
    }
}

// 访问者模式实现
void SemanticAnalyzer::visit(LiteralExpression &expr)
{
    // 字面量表达式不需要特殊的语义检查
}

void SemanticAnalyzer::visit(IdentifierExpression &expr)
{
    // 标识符表达式在getExpressionType中已经进行了检查
}

void SemanticAnalyzer::visit(BinaryExpression &expr)
{
    // 递归检查左右子表达式
    expr.getLeft()->accept(*this);
    expr.getRight()->accept(*this);

    // 类型检查在getExpressionType中已经进行
}
void SemanticAnalyzer::visit(AggregateExpression &expr)
{
    // 检查聚合函数的列是否存在
    if (!currentTable_ || currentTable_->table_name.empty())
    {
        throw SemanticError(SemanticError::ErrorType::UNKNOWN,
                            SqlErrors::noCurrentTableForIdentifier(expr.getColumn()));
    }
    checkColumnExists(currentTable_->table_name, expr.getColumn());
}

void SemanticAnalyzer::visit(CreateTableStatement &stmt)
{
    Logger logger("logs/semantic.log");
    logger.log(std::string("[Semantic] CreateTable: ") + stmt.getTableName());
    // 检查表是否已存在
    checkTableNotExists(stmt.getTableName());

    // 准备列定义
    std::vector<Column> columns;
    for (const auto &col : stmt.getColumns())
    {
        Column catalogCol;
        catalogCol.name = col.getName();
        // 转换数据类型
        if (col.getType() == ColumnDefinition::DataType::INT)
        {
            catalogCol.type = "INT";
        }
        else if (col.getType() == ColumnDefinition::DataType::VARCHAR)
        {
            catalogCol.type = "VARCHAR";
        }
        catalogCol.length = -1; // 默认长度
        // 约束映射
        if (col.isPrimaryKey()) { catalogCol.is_primary_key = true; catalogCol.not_null = true; }
        if (col.isUnique()) { catalogCol.is_unique = true; }
        if (col.isNotNull()) { catalogCol.not_null = true; }
        if (!col.getDefaultValue().empty()) { catalogCol.default_value = col.getDefaultValue(); }
        columns.push_back(catalogCol);
    }

    // 语义分析阶段不执行CREATE TABLE，只检查语义
    // 实际的CREATE TABLE操作在执行阶段进行
    logger.log(std::string("[Semantic] CreateTable semantic check finished: ") + stmt.getTableName());
}

void SemanticAnalyzer::visit(InsertStatement &stmt)
{
    Logger logger("logs/semantic.log");
    logger.log(std::string("[Semantic] Insert into: ") + stmt.getTableName());
    // 检查表是否存在
    checkTableExists(stmt.getTableName());
    currentTable_ = std::make_unique<TableSchema>(catalog_->GetTable(stmt.getTableName()));

    // 处理列名：如果没有指定列名，使用表的所有列
    std::vector<std::string> columnNames = stmt.getColumnNames();
    if (columnNames.empty())
    {
        // 如果没有指定列名，使用表的所有列
        for (const auto &col : currentTable_->columns)
        {
            columnNames.push_back(col.name);
        }
    }
    else
    {
        // 检查指定的列名是否存在
        for (const auto &colName : columnNames)
        {
            checkColumnExists(stmt.getTableName(), colName);
        }
    }

    // 检查列数是否匹配
    size_t expectedColumnCount = columnNames.size();
    for (const auto &valueList : stmt.getValueLists())
    {
        if (valueList.getValues().size() != expectedColumnCount)
        {
            throw SemanticError(SemanticError::ErrorType::COLUMN_COUNT_MISMATCH,
                                SqlErrors::columnCountMismatch(expectedColumnCount, valueList.getValues().size()));
        }

        // 检查每个值的类型是否与列类型匹配
        for (size_t i = 0; i < expectedColumnCount; ++i)
        {
            const std::string &colName = columnNames[i];

            // 查找列类型
            std::string colType;
            for (const auto &col : currentTable_->columns)
            {
                if (col.name == colName)
                {
                    colType = col.type;
                    break;
                }
            }

            Expression *valueExpr = valueList.getValues()[i].get();

            // 递归检查表达式
            valueExpr->accept(*this);

            // 检查类型兼容性
            std::string valueType = getExpressionType(valueExpr);
            checkTypeCompatibility(colType, valueType,
                                   "INSERT value for column " + colName);
        }
    }

    // 清空当前表
    currentTable_.reset();
    logger.log(std::string("[Semantic] Insert checks passed for: ") + stmt.getTableName());
}

void SemanticAnalyzer::visit(SelectStatement &stmt)
{
    Logger logger("logs/semantic.log");
    logger.log(std::string("[Semantic] Select from: ") + stmt.getTableName());
    // 检查表是否存在
    checkTableExists(stmt.getTableName());
    currentTable_ = std::make_unique<TableSchema>(catalog_->GetTable(stmt.getTableName()));

    // 检查列名是否存在
    // for (const auto& colName : stmt.getColumns()) {
    //     checkColumnExists(stmt.getTableName(), colName);
    //     // 检查列名是否存在（当为 * 时表示选择所有列，跳过检查）
    const auto &selectColumns = stmt.getColumns();
    bool isSelectAll = (selectColumns.size() == 1 && selectColumns[0] == "*");
    if (!isSelectAll)
    {
        for (const auto &colName : selectColumns)
        {
            checkColumnExists(stmt.getTableName(), colName);
        }
    }

    // 检查WHERE子句
    if (stmt.getWhereClause())
    {
        stmt.getWhereClause()->accept(*this);

        // WHERE子句的结果应该是布尔类型（在我们的简化模型中用INT表示）
        std::string whereType = getExpressionType(stmt.getWhereClause());
        if (whereType != "INT")
        {
            throw SemanticError(SemanticError::ErrorType::TYPE_MISMATCH,
                                "WHERE clause must evaluate to a boolean condition");
        }
    }

    // 清空当前表
    currentTable_.reset();
    logger.log(std::string("[Semantic] Select checks passed for: ") + stmt.getTableName());
}

void SemanticAnalyzer::visit(DeleteStatement &stmt)
{
    Logger logger("logs/semantic.log");
    logger.log(std::string("[Semantic] Delete from: ") + stmt.getTableName());
    // 检查表是否存在
    checkTableExists(stmt.getTableName());
    currentTable_ = std::make_unique<TableSchema>(catalog_->GetTable(stmt.getTableName()));

    // 检查WHERE子句
    if (stmt.getWhereClause())
    {
        stmt.getWhereClause()->accept(*this);

        // WHERE子句的结果应该是布尔类型（在我们的简化模型中用INT表示）
        std::string whereType = getExpressionType(stmt.getWhereClause());
        if (whereType != "INT")
        {
            throw SemanticError(SemanticError::ErrorType::TYPE_MISMATCH,
                                "WHERE clause must evaluate to a boolean condition");
        }
    }

    // 清空当前表
    currentTable_.reset();
    logger.log(std::string("[Semantic] Delete checks passed for: ") + stmt.getTableName());
}

void SemanticAnalyzer::visit(UpdateStatement &stmt)
{
    Logger logger("logs/semantic.log");
    logger.log(std::string("[Semantic] Update on: ") + stmt.getTableName());
    // 检查表是否存在
    checkTableExists(stmt.getTableName());
    currentTable_ = std::make_unique<TableSchema>(catalog_->GetTable(stmt.getTableName()));

    // 检查SET子句
    for (const auto &assignment : stmt.getAssignments())
    {
        const std::string &colName = assignment.first;
        checkColumnExists(stmt.getTableName(), colName);
        assignment.second->accept(*this);
    }

    // 检查WHERE子句
    if (stmt.getWhereClause())
    {
        stmt.getWhereClause()->accept(*this);

        // WHERE子句的结果应该是布尔类型（在我们的简化模型中用INT表示）
        std::string whereType = getExpressionType(stmt.getWhereClause());
        if (whereType != "INT")
        {
            throw SemanticError(SemanticError::ErrorType::TYPE_MISMATCH,
                                "WHERE clause must evaluate to a boolean condition");
        }
    }

    // 清空当前表
    currentTable_.reset();
    logger.log(std::string("[Semantic] Update checks passed for: ") + stmt.getTableName());
}
void SemanticAnalyzer::visit(ShowTablesStatement &stmt)
{
    Logger logger("logs/semantic.log");
    logger.log("[Semantic] ShowTables statement - no semantic checks needed.");
    // SHOW TABLES 语句不需要复杂的语义检查
    // 只需确保 Catalog 存在即可
    if (!catalog_)
    {
        throw SemanticError(SemanticError::ErrorType::UNKNOWN, "Catalog is not set for SHOW TABLES");
    }
    logger.log("[Semantic] ShowTables checks passed.");
}
void SemanticAnalyzer::visit(DropStatement& stmt) {
    Logger logger("logs/semantic.log");
    logger.log(std::string("[Semantic] Drop table: ") + stmt.getTableName());
    // 检查表是否存在
    checkTableExists(stmt.getTableName());
    logger.log(std::string("[Semantic] Drop table checks passed for: ") + stmt.getTableName());
}

void SemanticAnalyzer::visit(CreateIndexStatement &stmt)
{
    Logger logger("logs/semantic.log");
    logger.log(std::string("[Semantic] CreateIndex on table: ") + stmt.getTableName());
    // 基础检查：表存在
    checkTableExists(stmt.getTableName());
    // 列存在
    for (const auto &col : stmt.getColumns())
    {
        checkColumnExists(stmt.getTableName(), col);
    }
    logger.log("[Semantic] CreateIndex checks passed.");
}

void SemanticAnalyzer::visit(CallProcedureStatement &stmt)
{
    Logger logger("logs/semantic.log");
    logger.log(std::string("[Semantic] Call procedure: ") + stmt.getProcName());
    // 对 CALL 暂不进行表/列校验，由执行阶段通过 Catalog 检查过程是否存在
    // 可选：若需要，这里也可接入 catalog_->HasProcedure 检查
    if (!catalog_)
    {
        throw SemanticError(SemanticError::ErrorType::UNKNOWN, "Catalog is not set for CALL");
    }
}

void SemanticAnalyzer::visit(CreateProcedureStatement &stmt)
{
    Logger logger("logs/semantic.log");
    logger.log(std::string("[Semantic] Create procedure: ") + stmt.getProcName());
    // CREATE PROCEDURE 暂不进行复杂的语义检查，由执行阶段处理
    // 可选：这里可以检查过程名是否已存在
    if (!catalog_)
    {
        throw SemanticError(SemanticError::ErrorType::UNKNOWN, "Catalog is not set for CREATE PROCEDURE");
    }
}