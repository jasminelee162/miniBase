#pragma once
// #include "../../catalog/catalog.h" // Column
#include "../../catalog/catalog.h" // Column
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
    GroupBy,
    Having,
    Join,
    OrderBy // 新增
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
    std::vector<std::unique_ptr<PlanNode>> children;
    std::string table_name;
    std::vector<std::string> from_tables;
    std::vector<std::string> columns;
    std::vector<minidb::Column> table_columns;

    std::string predicate;
    std::vector<std::vector<std::string>> values;
    std::map<std::string, std::string> set_values;

    std::vector<std::string> group_keys;
    std::vector<AggregateExpr> aggregates;
    std::string having_predicate;

    // 新增字段：OrderBy
    std::vector<std::string> order_by_cols; // 按哪些列排序
    bool order_by_desc{false};              // 是否降序
};
