#pragma once
// #include "../../catalog/catalog.h" // Column
#include "catalog/catalog.h"

#include <string>
#include <vector>
#include <memory>
#include <map>

namespace minidb
{
    struct Column;
} // 前向声明

enum class PlanType
{
    SeqScan,
    Filter,
    Project,
    CreateTable,
    Insert,
    Delete,
    Update,
    GroupBy, // 新增
    Having   // 新增
};

struct AggregateExpr
{
    std::string func;    // 聚合函数，如 COUNT / SUM / AVG
    std::string column;  // 聚合的列
    std::string as_name; // 聚合结果的别名
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

    // 新增字段
    std::vector<std::string> group_keys;   // Group By 的列
    std::vector<AggregateExpr> aggregates; // 聚合表达式
    std::string having_predicate;          // Having 条件
};
