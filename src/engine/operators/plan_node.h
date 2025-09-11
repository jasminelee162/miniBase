#pragma once
#include "../../catalog/catalog.h" // Column
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
    std::vector<std::unique_ptr<PlanNode>> children; // 子节点
    std::string table_name;

    // 统一：所有节点只保存列名
    std::vector<std::string> columns;

    // 新增：用于建表，保存列完整信息
    std::vector<minidb::Column> table_columns;

    std::string predicate;                         // where 条件
    std::vector<std::vector<std::string>> values;  // Insert 用：每行数据
    std::map<std::string, std::string> set_values; // Update 用：列=值
};
