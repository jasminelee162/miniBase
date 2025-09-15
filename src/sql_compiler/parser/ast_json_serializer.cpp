#include "ast_json_serializer.h"
#include "ast.h"
#include <stdexcept>
#include "../common/error_messages.h"
#include <iostream>

using json = nlohmann::json;

static json exprToJson(const Expression* e);

json ASTJson::toJson(const Statement* stmt)
{
    if (!stmt) return json();

    // try dynamic cast to each statement type
    // CREATE TABLE
    if (auto ct = dynamic_cast<const CreateTableStatement*>(stmt)) {
        json j;
        j["type"] = "CreateTable";
        j["table_name"] = ct->getTableName();
        // columns: array of objects {name,type,length}
        json cols = json::array();
        for (auto &c : ct->getColumns()) {
            json cobj;
            cobj["name"] = c.getName();
            cobj["type"] = (c.getType() == ColumnDefinition::DataType::INT) ? "INT" : "VARCHAR";
            // 当前 AST 未保存长度信息，这里默认 0（VARCHAR(n) 可在后续扩展写入）
            cobj["length"] = 0;
            cols.push_back(cobj);
        }
        j["columns"] = cols;
        return j;
    }
    // INSERT
    if (auto ins = dynamic_cast<const InsertStatement*>(stmt)) {
        json j;
        j["type"] = "Insert";
        j["table_name"] = ins->getTableName();
        // 列名列表
        j["columns"] = ins->getColumnNames();
        // values: flatten first value list into array-of-arrays (single-row)
        json vals = json::array();
        // 处理所有值列表
        for (const auto& valueList : ins->getValueLists()) {
            json row = json::array();
            for (const auto& value : valueList.getValues()) {
                auto je = exprToJson(value.get());
                row.push_back(je.is_string() ? je.get<std::string>() : je.dump());
            }
            vals.push_back(row);
        }
        j["values"] = vals;
        return j;
    }
    // SELECT
    if (auto sel = dynamic_cast<const SelectStatement*>(stmt)) {
        json j;
        // 检查是否有 JOIN
        if (sel->hasJoins()) {
            j["type"] = "Join";
            j["from_tables"] = sel->getFromTables();
            
            // 假设只有一个 JOIN（可以扩展支持多个）
            if (!sel->getJoins().empty()) {
                j["predicate"] = sel->getJoins()[0].condition;
            }
            
            // 创建子节点结构
            json child;
            child["type"] = "SeqScan";
            child["table_name"] = sel->getMainTableName();
            j["child"] = child;
            
            // 创建 children 数组
            json children = json::array();
            for (const auto& joinClause : sel->getJoins()) {
                json joinChild;
                joinChild["type"] = "SeqScan";
                joinChild["table_name"] = joinClause.tableName;
                children.push_back(joinChild);
            }
            j["children"] = children;
            
            return j;
        }

        // 检查是否有 ORDER BY
        if (!sel->getOrderByColumns().empty()) {
            // 如果有 ORDER BY，创建嵌套结构
            j["type"] = "OrderBy";
            j["order_by_cols"] = sel->getOrderByColumns();
            j["order_by_desc"] = sel->isOrderByDesc();
            
            // 创建子节点
            json child;
            
            // 检查是否有 GROUP BY
            if (!sel->getGroupByColumns().empty()) {
                child["type"] = "GroupBy";
                child["table_name"] = sel->getTableName();
                child["group_keys"] = sel->getGroupByColumns();
                
                // 处理聚合函数
                json aggregatesJson = json::array();
                for (const auto& agg : sel->getAggregates()) {
                    json aggObj;
                    aggObj["func"] = agg->getFunction();
                    aggObj["column"] = agg->getColumn();
                    if (!agg->getAlias().empty()) {
                        aggObj["as"] = agg->getAlias();
                    }
                    aggregatesJson.push_back(aggObj);
                }
                child["aggregates"] = aggregatesJson;
                
                // HAVING 子句
                if (sel->getHavingClause()) {
                    try {
                        auto havingJson = exprToJson(sel->getHavingClause());
                        child["having_predicate"] = havingJson.is_string() ? havingJson.get<std::string>() : havingJson.dump();
                    } catch (const std::exception &e) {
                        child["having_predicate"] = "HAVING_CLAUSE_ERROR";
                    }
                }
            } else {
                // 没有 GROUP BY，创建 SeqScan 子节点
                child["type"] = "SeqScan";
                child["table_name"] = sel->getTableName();
                
                // 如果有 WHERE 子句，可能需要 Filter 节点
                if (sel->getWhereClause()) {
                    // 这里简化处理，实际可能需要更复杂的逻辑
                    try {
                        auto whereJson = exprToJson(sel->getWhereClause());
                        child["predicate"] = whereJson.is_string() ? whereJson.get<std::string>() : whereJson.dump();
                    } catch (const std::exception &e) {
                        child["predicate"] = "WHERE_CLAUSE_ERROR";
                    }
                }
            }
            
            j["child"] = child;
            return j;
        }
        // 如果没有 ORDER BY，使用原有的逻辑
        //先处理聚合函数group by
        if (!sel->getGroupByColumns().empty()) {
            j["type"] = "GroupBy";
            j["table_name"] = sel->getTableName();
            j["group_keys"] = sel->getGroupByColumns();
            
            // 处理聚合函数
            json aggregatesJson = json::array();
            for (const auto& agg : sel->getAggregates()) {
                json aggObj;
                aggObj["func"] = agg->getFunction();
                aggObj["column"] = agg->getColumn();
                if (!agg->getAlias().empty()) {
                    aggObj["as"] = agg->getAlias();
                }
                aggregatesJson.push_back(aggObj);
            }
            j["aggregates"] = aggregatesJson;
        
            // 处理 HAVING 子句
            if (sel->getHavingClause()) {
                try {
                    auto havingJson = exprToJson(sel->getHavingClause());
                    j["having_predicate"] = havingJson.is_string() ? havingJson.get<std::string>() : havingJson.dump();
                } catch (const std::exception &e) {
                    std::cerr << "[ASTJson][ERROR] 序列化 HAVING 子句失败: " << e.what() << std::endl;
                    j["having_predicate"] = "HAVING_CLAUSE_ERROR";
                }
            }
        } 
        else {
            // 普通的 SELECT
            j["type"] = "Select";
            j["table_name"] = sel->getTableName();

            //处理列名
            auto columns = sel->getColumns();
            if (columns.size() == 1 && columns[0] == "*") {
                std::cout << "[ASTJson] 序列化 SELECT *" << std::endl;
                j["columns"] = json::array({"*"});
            } else {
                j["columns"] = columns;
            }

            //处理 WHERE 子句
            if (sel->getWhereClause()) {
                std::cout << "[ASTJson] 序列化 WHERE 子句" << std::endl;
                try{
                auto p = exprToJson(sel->getWhereClause());
                j["predicate"] = p.is_string() ? p.get<std::string>() : p.dump();
                }  
                catch (const std::exception &e) {
                    std::cerr << "[ASTJson][ERROR] 序列化 WHERE 子句失败: " << e.what() << std::endl;
                    j["predicate"] = "WHERE_CLAUSE_ERROR";
                }
            }
        
        }
    return j;
    }

    //Delete
    if (auto del = dynamic_cast<const DeleteStatement*>(stmt)) {
        json j;
        j["type"] = "Delete";
        j["table_name"] = del->getTableName();
        if (del->getWhereClause()) {
            auto p = exprToJson(del->getWhereClause());
            j["predicate"] = p.is_string() ? p.get<std::string>() : p.dump();
        }
        return j;
    }
    //Update
    if (auto upd = dynamic_cast<const UpdateStatement*>(stmt)) {
        json j;
        j["type"] = "Update";
        j["table_name"] = upd->getTableName();
        // 处理SET子句 → set_values 对象
        json set_values = json::object();
        for (const auto& assignment : upd->getAssignments()) {
            auto exprJson = exprToJson(assignment.second.get());
            // 统一转为字符串
            std::string valueStr = exprJson.is_string() ? exprJson.get<std::string>() : exprJson.dump();
            set_values[assignment.first] = valueStr;
        }
        j["set_values"] = set_values;
        // 处理WHERE子句
        if (upd->getWhereClause()) {
            auto p = exprToJson(upd->getWhereClause());
            j["predicate"] = p.is_string() ? p.get<std::string>() : p.dump();
        }
        return j;
    }
    //SHOW
    if(auto show = dynamic_cast<const ShowTablesStatement*>(stmt)){
        json j;
        j["type"] = "ShowTables";
        return j;

    }
    //DROP
    if(auto drop = dynamic_cast<const DropStatement*>(stmt)){
        json j;
        j["type"] = "Drop";
        j["table_name"] = drop->getTableName();
        return j;
    }
    // CALL PROCEDURE
    if (auto call = dynamic_cast<const CallProcedureStatement*>(stmt)) {
        json j;
        j["type"] = "CallProcedure";
        j["proc_name"] = call->getProcName();
        j["proc_args"] = call->getArgs();
        return j;
    }
    throw std::runtime_error(SqlErrors::UNSUPPORTED_STMT_JSON);
}

static json exprToJson(const Expression* e)
{
    if (!e) return json();
    if (auto lit = dynamic_cast<const LiteralExpression*>(e)) {
        if (lit->getType() == LiteralExpression::LiteralType::INTEGER)
            return std::string(lit->getValue());
        else
            return std::string(lit->getValue());
    }
    if (auto id = dynamic_cast<const IdentifierExpression*>(e)) {
        return std::string(id->getName());
    }
    if (auto be = dynamic_cast<const BinaryExpression*>(e)) {
        // simple infix string representation
        json j;
        std::string left = exprToJson(be->getLeft()).is_string() ? exprToJson(be->getLeft()).get<std::string>() : "";
        std::string right = exprToJson(be->getRight()).is_string() ? exprToJson(be->getRight()).get<std::string>() : "";
        std::string op;
        switch (be->getOperator()) {
            case BinaryExpression::Operator::EQUALS: op = " = "; break;
            case BinaryExpression::Operator::LESS_THAN: op = " < "; break;
            case BinaryExpression::Operator::GREATER_THAN: op = " > "; break;
            case BinaryExpression::Operator::LESS_EQUAL: op = " <= "; break;
            case BinaryExpression::Operator::GREATER_EQUAL: op = " >= "; break;
            case BinaryExpression::Operator::NOT_EQUAL: op = " != "; break;
            case BinaryExpression::Operator::PLUS: op = " + "; break;
            case BinaryExpression::Operator::MINUS: op = " - "; break;
            case BinaryExpression::Operator::MULTIPLY: op = " * "; break;
            case BinaryExpression::Operator::DIVIDE: op = " / "; break;
        }
        return left + op + right;
    }
    if( auto ae = dynamic_cast<const AggregateExpression*>(e)) {
        json j;
        j["function"] = ae->getFunction();
        j["column"] = ae->getColumn();
        if (!ae->getAlias().empty()) {
            j["as"] = ae->getAlias();
        }
        return j;
    }

    return json();
}
