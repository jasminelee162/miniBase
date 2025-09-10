#include "planner.h"
#include <stdexcept>
#include <sstream>

// 将表达式转换为字符串值
std::string Planner::expressionToString(Expression* expr) {
    if (!expr) {
        throw PlannerError(PlannerError::ErrorType::MISSING_SEMANTIC_INFO, "Expression is null");
    }
    
    if (auto literal = dynamic_cast<LiteralExpression*>(expr)) {
        return literal->getValue();
    }
    
    throw PlannerError(PlannerError::ErrorType::UNSUPPORTED_FEATURE, 
                      "Only literal expressions can be converted to values");
}

// 将表达式转换为谓词字符串
std::string Planner::expressionToPredicate(Expression* expr) {
    if (!expr) {
        return "";
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
    std::string op;
    switch (binaryExpr->getOperator()) {
        case BinaryExpression::Operator::EQUALS:
            op = "=";
            break;
        case BinaryExpression::Operator::LESS_THAN:
            op = "<";
            break;
        case BinaryExpression::Operator::GREATER_THAN:
            op = ">";
            break;
        case BinaryExpression::Operator::LESS_EQUAL:
            op = "<=";
            break;
        case BinaryExpression::Operator::GREATER_EQUAL:
            op = ">=";
            break;
        case BinaryExpression::Operator::NOT_EQUAL:
            op = "!=";
            break;
        default:
            throw PlannerError(PlannerError::ErrorType::UNSUPPORTED_FEATURE, 
                             "Unsupported operator in predicate");
    }
    
    // 创建谓词字符串
    std::stringstream ss;
    ss << identifier->getName() << " " << op << " " << expressionToString(right);
    return ss.str();
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
    // 创建CREATE TABLE计划节点
    currentPlan = std::make_unique<PlanNode>();
    currentPlan->type = PlanType::CreateTable;
    currentPlan->table_name = stmt.getTableName();
    
    // 添加列信息
    for (const auto& col : stmt.getColumns()) {
        currentPlan->columns.push_back(col.getName());
    }
}

// 访问INSERT语句
void Planner::visit(InsertStatement& stmt) {
    // 创建INSERT计划节点
    currentPlan = std::make_unique<PlanNode>();
    currentPlan->type = PlanType::Insert;
    currentPlan->table_name = stmt.getTableName();
    
    // 添加列名
    currentPlan->columns = stmt.getColumnNames();
    
    // 添加值列表
    for (const auto& valueList : stmt.getValueLists()) {
        std::vector<std::string> values;
        for (const auto& expr : valueList.getValues()) {
            values.push_back(expressionToString(expr.get()));
        }
        currentPlan->values.push_back(std::move(values));
    }
}

// 访问SELECT语句
void Planner::visit(SelectStatement& stmt) {
    // 如果有WHERE子句，创建Filter节点
    if (stmt.getWhereClause()) {
        // 创建Filter计划节点
        auto filterNode = std::make_unique<PlanNode>();
        filterNode->type = PlanType::Filter;
        filterNode->table_name = stmt.getTableName();
        filterNode->predicate = expressionToPredicate(stmt.getWhereClause());
        
        // 创建SeqScan计划节点作为Filter的子节点
        auto scanNode = std::make_unique<PlanNode>();
        scanNode->type = PlanType::SeqScan;
        scanNode->table_name = stmt.getTableName();
        
        // 将SeqScan添加为Filter的子节点
        filterNode->children.push_back(std::move(scanNode));
        
        // 如果有投影列，创建Project节点
        if (!stmt.getColumns().empty() && !(stmt.getColumns().size() == 1 && stmt.getColumns()[0] == "*")) {
            // 创建Project计划节点
            currentPlan = std::make_unique<PlanNode>();
            currentPlan->type = PlanType::Project;
            currentPlan->columns = stmt.getColumns();
            
            // 将Filter添加为Project的子节点
            currentPlan->children.push_back(std::move(filterNode));
        } else {
            // 否则直接使用Filter节点
            currentPlan = std::move(filterNode);
        }
    } else {
        // 没有WHERE子句，直接创建SeqScan节点
        auto scanNode = std::make_unique<PlanNode>();
        scanNode->type = PlanType::SeqScan;
        scanNode->table_name = stmt.getTableName();
        
        // 如果有投影列，创建Project节点
        if (!stmt.getColumns().empty() && !(stmt.getColumns().size() == 1 && stmt.getColumns()[0] == "*")) {
            // 创建Project计划节点
            currentPlan = std::make_unique<PlanNode>();
            currentPlan->type = PlanType::Project;
            currentPlan->columns = stmt.getColumns();
            
            // 将SeqScan添加为Project的子节点
            currentPlan->children.push_back(std::move(scanNode));
        } else {
            // 否则直接使用SeqScan节点
            currentPlan = std::move(scanNode);
        }
    }
}

// 访问DELETE语句
void Planner::visit(DeleteStatement& stmt) {
    // 创建DELETE计划节点
    currentPlan = std::make_unique<PlanNode>();
    currentPlan->type = PlanType::Delete;
    currentPlan->table_name = stmt.getTableName();
    
    // 如果有WHERE子句，添加谓词
    if (stmt.getWhereClause()) {
        currentPlan->predicate = expressionToPredicate(stmt.getWhereClause());
    }
}