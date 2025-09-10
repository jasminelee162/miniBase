#include "planner.h"
#include <stdexcept>

// 将表达式转换为值
Value Planner::expressionToValue(Expression* expr) {
    if (!expr) {
        throw PlannerError(PlannerError::ErrorType::MISSING_SEMANTIC_INFO, "Expression is null");
    }
    
    if (auto literal = dynamic_cast<LiteralExpression*>(expr)) {
        if (literal->getType() == LiteralExpression::LiteralType::INTEGER) {
            return Value(std::stoi(literal->getValue()));
        } else if (literal->getType() == LiteralExpression::LiteralType::STRING) {
            return Value(literal->getValue());
        }
    }
    
    throw PlannerError(PlannerError::ErrorType::UNSUPPORTED_FEATURE, 
                      "Only literal expressions can be converted to values");
}

// 将表达式转换为谓词
std::unique_ptr<Predicate> Planner::expressionToPredicate(Expression* expr) {
    if (!expr) {
        return nullptr;
    }
    
    auto binaryExpr = dynamic_cast<BinaryExpression*>(expr);
    if (!binaryExpr) {
        throw PlannerError(PlannerError::ErrorType::UNSUPPORTED_FEATURE, 
                         "Only binary expressions can be converted to predicates");
    }
    
    auto left = binaryExpr->getLeft();
    auto right = binaryExpr->getRight();
    
    // 确保左侧是标识符，右侧是字面量
    auto identifier = dynamic_cast<IdentifierExpression*>(left);
    if (!identifier) {
        throw PlannerError(PlannerError::ErrorType::UNSUPPORTED_FEATURE, 
                         "Left side of predicate must be a column identifier");
    }
    
    // 转换操作符
    Predicate::Operator op;
    switch (binaryExpr->getOperator()) {
        case BinaryExpression::Operator::EQUALS:
            op = Predicate::Operator::EQUALS;
            break;
        case BinaryExpression::Operator::LESS_THAN:
            op = Predicate::Operator::LESS_THAN;
            break;
        case BinaryExpression::Operator::GREATER_THAN:
            op = Predicate::Operator::GREATER_THAN;
            break;
        case BinaryExpression::Operator::LESS_EQUAL:
            op = Predicate::Operator::LESS_EQUAL;
            break;
        case BinaryExpression::Operator::GREATER_EQUAL:
            op = Predicate::Operator::GREATER_EQUAL;
            break;
        case BinaryExpression::Operator::NOT_EQUAL:
            op = Predicate::Operator::NOT_EQUAL;
            break;
        default:
            throw PlannerError(PlannerError::ErrorType::UNSUPPORTED_FEATURE, 
                             "Unsupported operator in predicate");
    }
    
    // 创建谓词
    return std::make_unique<Predicate>(identifier->getName(), op, expressionToValue(right));
}

// 访问字面量表达式
void Planner::visit(LiteralExpression& expr) {
    // 字面量表达式通常作为其他表达式的一部分处理
    // 不需要单独生成计划
}

// 访问标识符表达式
void Planner::visit(IdentifierExpression& expr) {
    // 标识符表达式通常作为其他表达式的一部分处理
    // 不需要单独生成计划
}

// 访问二元表达式
void Planner::visit(BinaryExpression& expr) {
    // 二元表达式通常作为谓词的一部分处理
    // 不需要单独生成计划
}

// 访问CREATE TABLE语句
void Planner::visit(CreateTableStatement& stmt) {
    std::vector<std::pair<std::string, ColumnInfo::DataType>> columns;
    
    for (const auto& col : stmt.getColumns()) {
        ColumnInfo::DataType type;
        switch (col.getType()) {
            case ColumnDefinition::DataType::INT:
                type = ColumnInfo::DataType::INT;
                break;
            case ColumnDefinition::DataType::VARCHAR:
                type = ColumnInfo::DataType::VARCHAR;
                break;
            default:
                throw PlannerError(PlannerError::ErrorType::UNSUPPORTED_FEATURE, 
                                 "Unsupported column type");
        }
        columns.emplace_back(col.getName(), type);
    }
    
    currentPlan = std::make_unique<CreateTablePlanNode>(stmt.getTableName(), columns);
}

// 访问INSERT语句
void Planner::visit(InsertStatement& stmt) {
    std::vector<std::vector<Value>> valuesList;
    
    for (const auto& valueList : stmt.getValueLists()) {
        std::vector<Value> values;
        for (const auto& expr : valueList.getValues()) {
            values.push_back(expressionToValue(expr.get()));
        }
        valuesList.push_back(std::move(values));
    }
    
    currentPlan = std::make_unique<InsertPlanNode>(stmt.getTableName(), 
                                                 stmt.getColumnNames(), 
                                                 valuesList);
}

// 访问SELECT语句
void Planner::visit(SelectStatement& stmt) {
    // 创建表扫描节点
    auto scanNode = std::make_unique<SeqScanPlanNode>(
        stmt.getTableName(), 
        expressionToPredicate(stmt.getWhereClause())
    );
    
    // 如果有投影列，创建投影节点
    if (!stmt.getColumns().empty() && !(stmt.getColumns().size() == 1 && stmt.getColumns()[0] == "*")) {
        currentPlan = std::make_unique<ProjectPlanNode>(std::move(scanNode), stmt.getColumns());
    } else {
        // 否则直接使用表扫描节点
        currentPlan = std::move(scanNode);
    }
}

// 访问DELETE语句
void Planner::visit(DeleteStatement& stmt) {
    currentPlan = std::make_unique<DeletePlanNode>(
        stmt.getTableName(), 
        expressionToPredicate(stmt.getWhereClause())
    );
}