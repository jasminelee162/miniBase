#include "ast_json_serializer.h"
#include "ast.h"
#include <stdexcept>

using json = nlohmann::json;

static json exprToJson(const Expression* e);

json ASTJson::toJson(const Statement* stmt)
{
    if (!stmt) return json();

    // try dynamic cast to each statement type
    if (auto ct = dynamic_cast<const CreateTableStatement*>(stmt)) {
        json j;
        j["type"] = "CreateTable";
        j["table"] = ct->getTableName();
        json schema = json::array();
        for (auto &c : ct->getColumns()) {
            schema.push_back({{"name", c.getName()}, {"type", c.getType() == ColumnDefinition::DataType::INT ? "int" : "varchar"}});
        }
        j["schema"] = schema;
        return j;
    }

    if (auto ins = dynamic_cast<const InsertStatement*>(stmt)) {
        json j;
        j["type"] = "Insert";
        j["table"] = ins->getTableName();
        // flatten first value list into simple array of strings (DB expects single-row values array)
        json vals = json::array();
        if (!ins->getValueLists().empty()) {
            auto &vl = ins->getValueLists().front();
            for (auto &v : vl.getValues()) {
                auto je = exprToJson(v.get());
                if (je.is_string()) vals.push_back(je.get<std::string>());
                else vals.push_back(je.dump());
            }
        }
        j["values"] = vals;
        return j;
    }

    if (auto sel = dynamic_cast<const SelectStatement*>(stmt)) {
        json j;
        j["type"] = "Select";
        j["table"] = sel->getTableName();
        j["columns"] = sel->getColumns();
        if (sel->getWhereClause()) {
            auto p = exprToJson(sel->getWhereClause());
            j["predicate"] = p.is_string() ? p.get<std::string>() : p.dump();
        } else {
            j["predicate"] = json();
        }
        return j;
    }

    if (auto del = dynamic_cast<const DeleteStatement*>(stmt)) {
        json j;
        j["type"] = "Delete";
        j["table"] = del->getTableName();
        if (del->getWhereClause()) {
            auto p = exprToJson(del->getWhereClause());
            j["predicate"] = p.is_string() ? p.get<std::string>() : p.dump();
        }
        return j;
    }

    throw std::runtime_error("Unsupported statement type for JSON serialization");
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

    return json();
}
