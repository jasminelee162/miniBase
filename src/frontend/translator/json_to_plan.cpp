#include "json_to_plan.h"
#include "../../engine/operators/plan_node.h" // path 根据你的工程实际路径调整
#include <stdexcept>
#include <iostream>
#include "../../util/logger.h"

using json = nlohmann::json;

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
                global_log_debug("[JsonToPlan] 处理 SELECT *");
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

        if (j.contains("order_by_cols") && j["order_by_cols"].is_array())
        {
            for (const auto &col : j["order_by_cols"])
            {
                node->order_by_cols.push_back(col.get<std::string>());
            }
        }

        if (j.contains("order_by_desc"))
        {
            node->order_by_desc = j["order_by_desc"].get<bool>();
        }
        // OrderBy 通常有一个子节点（Project/Filter/SeqScan）
        if (j.contains("child"))
            node->children.push_back(translate(j["child"]));
    }
    else if (type == "ShowTables")
    {
        node->type = PlanType::ShowTables;
        // SHOW TABLES 不需要额外参数
    }
    else if (type == "Drop")
    {
        node->type = PlanType::Drop;

        // DROP TABLE 一般 JSON 会有 table_name
        if (!j.contains("table_name"))
            throw std::runtime_error("Drop plan must have table_name");

        node->table_name = j["table_name"].get<std::string>();

        // 不需要子节点，也没有其他参数
        node->children.clear();
    }
    else if (type == "CreateProcedure")
    {
        node->type = PlanType::CreateProcedure;

        // 兼容两种键名：{name,params,body} 或 {proc_name,proc_params,proc_body}
        if (j.contains("name") && j["name"].is_string())
            node->proc_name = j["name"].get<std::string>();
        else if (j.contains("proc_name") && j["proc_name"].is_string())
            node->proc_name = j["proc_name"].get<std::string>();
        else
            throw std::runtime_error("CreateProcedure plan must have name");

        if (j.contains("params") && j["params"].is_array())
        {
            for (auto &param : j["params"])
                node->proc_params.push_back(param.get<std::string>());
        }
        else if (j.contains("proc_params") && j["proc_params"].is_array())
        {
            for (auto &param : j["proc_params"])
                node->proc_params.push_back(param.get<std::string>());
        }

        if (j.contains("body") && j["body"].is_string())
            node->proc_body = j["body"].get<std::string>();
        else if (j.contains("proc_body") && j["proc_body"].is_string())
            node->proc_body = j["proc_body"].get<std::string>();
        else
            throw std::runtime_error("CreateProcedure plan must have body");

        node->children.clear(); // 定义过程不需要子节点
    }
    else if (type == "CallProcedure")
    {
        node->type = PlanType::CallProcedure;

        // 兼容两种键名：{name,args} 或 {proc_name,proc_args}
        if (j.contains("name") && j["name"].is_string())
            node->proc_name = j["name"].get<std::string>();
        else if (j.contains("proc_name") && j["proc_name"].is_string())
            node->proc_name = j["proc_name"].get<std::string>();
        else
            throw std::runtime_error("CallProcedure plan must have name or proc_name");

        if (j.contains("args") && j["args"].is_array())
        {
            for (auto &arg : j["args"])
                node->proc_args.push_back(arg.get<std::string>());
        }
        else if (j.contains("proc_args") && j["proc_args"].is_array())
        {
            for (auto &arg : j["proc_args"])
                node->proc_args.push_back(arg.get<std::string>());
        }

        node->children.clear(); // 调用过程不需要子节点
    }
    else if (type == "CreateIndex")
    {
        node->type = PlanType::CreateIndex;

        // 索引名
        if (!j.contains("name"))
            throw std::runtime_error("CreateIndex plan must have name");
        node->index_name = j["name"].get<std::string>();

        // 表名
        if (!j.contains("table_name"))
            throw std::runtime_error("CreateIndex plan must have table_name");
        node->table_name = j["table_name"].get<std::string>();

        // 索引列
        if (!j.contains("columns"))
            throw std::runtime_error("CreateIndex plan must have columns");
        node->index_cols = j["columns"].get<std::vector<std::string>>();

        // 索引类型（可选，默认 BPLUS）
        node->index_type = j.value("index_type", "BPLUS");

        node->children.clear(); // 不需要子节点
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
                if (c.contains("is_primary_key"))
                    col.is_primary_key = c.at("is_primary_key").get<bool>();
                if (c.contains("is_unique"))
                    col.is_unique = c.at("is_unique").get<bool>();
                if (c.contains("not_null"))
                    col.not_null = c.at("not_null").get<bool>();
                if (c.contains("default_value"))
                    col.default_value = c.at("default_value").get<std::string>();
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
        for (auto it = j["set_values"].begin(); it != j["set_values"].end(); ++it)
        {
            node->set_values[it.key()] = it.value().get<std::string>();
        }
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
