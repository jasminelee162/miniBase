#include "json_to_plan.h"
#include "../../engine/operators/plan_node.h"
#include <stdexcept>
#include <iostream>

std::unique_ptr<PlanNode> JsonToPlan::translate(const json &j)
{
    auto node = std::make_unique<PlanNode>();

    // ----------- 类型 -----------
    std::string type = j.at("type").get<std::string>();

    if (type == "SeqScan")
        node->type = PlanType::SeqScan;
    else if (type == "Filter")
        node->type = PlanType::Filter;
    else if (type == "Project")
        node->type = PlanType::Project;
    else if (type == "CreateTable")
        node->type = PlanType::CreateTable;
    else if (type == "Insert")
        node->type = PlanType::Insert;
    else if (type == "Delete")
        node->type = PlanType::Delete;
    else if (type == "Update")
        node->type = PlanType::Update;
    else if (type == "Select")
    {
        // Select 特殊处理：拆成 Project(Filter(SeqScan))
        auto project = std::make_unique<PlanNode>();
        project->type = PlanType::Project;
        project->table_name = j.at("table_name").get<std::string>();

        if (j.contains("columns")) {
            auto columns = j["columns"];
            if (columns.is_array() && columns.size() == 1 && columns[0] == "*") {
                // SELECT * 的情况 - 不设置 columns，让执行器处理所有列
                std::cout << "[JsonToPlan] 处理 SELECT *" << std::endl;
                project->columns.clear(); // 空的 columns 表示选择所有列
            } else {
                // 具体列名的情况
                project->columns = j["columns"].get<std::vector<std::string>>();
            }
        }
        auto scan = std::make_unique<PlanNode>();
        scan->type = PlanType::SeqScan;
        scan->table_name = j.at("table_name").get<std::string>();

        if (j.contains("predicate"))
        {
            auto filter = std::make_unique<PlanNode>();
            filter->type = PlanType::Filter;
            filter->table_name = j.at("table_name").get<std::string>();
            filter->predicate = j.at("predicate").get<std::string>();
            filter->children.push_back(std::move(scan));
            project->children.push_back(std::move(filter));
        }
        else
        {
            project->children.push_back(std::move(scan));
        }

        return project;
    }
    else if (type == "GroupBy")
    {
        node->type = PlanType::GroupBy;

        if (j.contains("group_keys"))
            node->group_keys = j["group_keys"].get<std::vector<std::string>>();

        if (j.contains("aggregates"))
        {
            for (auto &agg : j["aggregates"])
            {
                AggregateExpr expr;
                expr.func = agg.at("func").get<std::string>();
                expr.column = agg.at("column").get<std::string>();
                expr.as_name = agg.value("as", "");
                node->aggregates.push_back(expr);
            }
        }

        // 添加 having_predicate 支持
        if (j.contains("having_predicate"))
            node->having_predicate = j["having_predicate"].get<std::string>();
    }
    else if (type == "Having")
    {
        node->type = PlanType::Having;
        if (j.contains("predicate"))
            node->predicate = j["predicate"].get<std::string>();
    }

    else
        throw std::runtime_error("Unknown plan type: " + type);

    // ----------- 通用字段 -----------
    if (j.contains("table_name"))
        node->table_name = j["table_name"].get<std::string>();

    if (j.contains("columns"))
    {
        if (node->type == PlanType::CreateTable)
        {
            // 保存完整列信息
            node->table_columns.clear();
            for (auto &c : j["columns"])
            {
                minidb::Column col;
                col.name = c.at("name").get<std::string>();
                col.type = c.at("type").get<std::string>();
                col.length = c.at("length").get<int>();
                node->table_columns.push_back(col);
            }
        }
        else
        {
            // 普通节点只保存列名
            node->columns = j["columns"].get<std::vector<std::string>>();
        }
    }

    if (j.contains("values"))
        node->values = j["values"].get<std::vector<std::vector<std::string>>>();

    if (j.contains("set_values"))
    {
        for (auto &[col, val] : j["set_values"].items())
            node->set_values[col] = val.get<std::string>();
    }

    if (j.contains("predicate"))
        node->predicate = j["predicate"].get<std::string>();

    // ----------- 子节点 -----------
    if (j.contains("child"))
        node->children.push_back(translate(j["child"]));
    if (j.contains("children"))
    {
        for (auto &child : j["children"])
            node->children.push_back(translate(child));
    }

    return node;
}
