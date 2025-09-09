#include "json_to_plan.h"
#include <stdexcept>
#include <iostream>

std::unique_ptr<PlanNode> JsonToPlan::translate(const json &j)
{
    auto node = std::make_unique<PlanNode>();

    std::string type = j["type"];
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

    if (j.contains("table_name"))
    {
        node->table_name = j["table_name"].get<std::string>();
    }
    if (j.contains("columns"))
    {
        node->columns = j["columns"].get<std::vector<std::string>>();
    }
    if (j.contains("predicate"))
    {
        node->predicate = j["predicate"].get<std::string>();
    }

    // 递归解析子节点
    if (j.contains("child"))
    {
        node->children.push_back(translate(j["child"])); // unique_ptr 直接push_back
    }
    if (j.contains("children"))
    {
        for (auto &child : j["children"])
        {
            node->children.push_back(translate(child));
        }
    }

    return node;
}
