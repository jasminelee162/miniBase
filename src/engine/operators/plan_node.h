#pragma once

#include "../../catalog/catalog.h" // Column
#include <string>
#include <vector>
#include <memory>
#include <map>

namespace minidb
{
    struct Column; // 前向声明（catalog.h 应该定义 Column）
} // namespace minidb

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
    OrderBy,
    ShowTables,
    Drop,
    CreateProcedure, // 新增：定义存储过程
    CallProcedure    // 新增：调用存储过程
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

    PlanNode() : type(PlanType::SeqScan) {} // 默认类型，构造时可改

    // === 存储过程相关 ===
    std::string proc_name;                // 存储过程名字
    std::vector<std::string> proc_params; // 参数列表
    std::vector<std::string> proc_args;   // 调用时的实参列表 ✅ 新增
    std::string proc_body;                // 过程体（SQL语句块）
};
