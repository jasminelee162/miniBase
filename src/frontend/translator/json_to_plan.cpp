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
        project->table_name = j.value("table_name", "");

        if (j.contains("from_tables"))
            project->from_tables = j["from_tables"].get<std::vector<std::string>>();
        else if (!project->table_name.empty())
            project->from_tables = {project->table_name};

        if (j.contains("columns"))
        {
            auto columns = j["columns"];
            if (columns.is_array() && columns.size() == 1 && columns[0] == "*")
            {
                // SELECT * 的情况 - 不设置 columns，让执行器处理所有列
                std::cout << "[JsonToPlan] 处理 SELECT *" << std::endl;
                project->columns.clear(); // 空的 columns 表示选择所有列
            }
            else
            {
                // 具体列名的情况
                project->columns = j["columns"].get<std::vector<std::string>>();
            }
        }
        auto scan = std::make_unique<PlanNode>();
        scan->type = PlanType::SeqScan;
        scan->table_name = project->table_name;
        scan->from_tables = project->from_tables;

        if (j.contains("predicate"))
        {
            auto filter = std::make_unique<PlanNode>();
            filter->type = PlanType::Filter;
            filter->table_name = project->table_name;
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
    else if (type == "Join")
    {
        node->type = PlanType::Join;

        // 读取连接表列表
        if (!j.contains("from_tables"))
            throw std::runtime_error("Join must have from_tables");
        node->from_tables = j["from_tables"].get<std::vector<std::string>>();
        if (node->from_tables.size() < 2)
            throw std::runtime_error("Join requires at least two tables");

        // 连接条件
        if (j.contains("predicate"))
            node->predicate = j["predicate"].get<std::string>();

        // 为每个表创建 SeqScan 子节点
        for (auto &tbl : node->from_tables)
        {
            auto scan = std::make_unique<PlanNode>();
            scan->type = PlanType::SeqScan;
            scan->table_name = tbl;
            scan->from_tables = {tbl};
            node->children.push_back(std::move(scan));
        }

        // 支持列投影
        if (j.contains("columns"))
            node->columns = j["columns"].get<std::vector<std::string>>();

        return node;
    }
    else if (type == "OrderBy")
    {
        node->type = PlanType::OrderBy;

        if (j.contains("order_keys"))
            node->order_by_cols = j["order_keys"].get<std::vector<std::string>>();

        if (j.contains("order") && j["order"].is_string())
        {
            std::string order = j["order"].get<std::string>();
            node->order_by_desc = (order == "DESC");
        }

        if (j.contains("order_by_cols") && j["order_by_cols"].is_array()) {
            for (const auto& col : j["order_by_cols"]) {
                node->order_by_cols.push_back(col.get<std::string>());
            }
        }
        
        if (j.contains("order_by_desc")) {
            node->order_by_desc = j["order_by_desc"].get<bool>();
        }
        // OrderBy 通常有一个子节点（Project/Filter/SeqScan）
        if (j.contains("child"))
            node->children.push_back(translate(j["child"]));
            
    }

    else
        throw std::runtime_error("Unknown plan type: " + type);

    // ----------- 通用字段 -----------
    if (j.contains("table_name"))
        node->table_name = j["table_name"].get<std::string>();

    if (j.contains("from_tables"))
        node->from_tables = j["from_tables"].get<std::vector<std::string>>();
    else if (!node->table_name.empty())
        node->from_tables = {node->table_name};

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
            // 普通节点只保存列名（可能是 "*"）
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
