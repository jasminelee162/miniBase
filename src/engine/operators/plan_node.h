#pragma once
#include <string>
#include <vector>
#include <memory>
#include <map>

enum class PlanType
{
    SeqScan,
    Filter,
    Project,
    CreateTable,
    Insert,
    Delete,
    Update
};

struct PlanNode
{
    PlanType type;
    std::vector<std::unique_ptr<PlanNode>> children; // 用智能指针管理子节点
    std::string table_name;
    std::vector<std::string> columns;
    std::string predicate;
    std::vector<std::vector<std::string>> values;  // 每行数据的字符串
    std::map<std::string, std::string> set_values; // Update 用
};
