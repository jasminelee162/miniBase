/**
 * 定义执行计划节点类型和结构
 */
#pragma once
#include <string>
#include <vector>

enum class PlanType
{
    SeqScan,
    Filter,
    Project,
    CreateTable,
    Insert,
    Delete
};

struct PlanNode
{
    PlanType type;
    std::vector<PlanNode *> children; // 子节点
    std::string table_name;           // 表名
    std::vector<std::string> columns; // SELECT列
    std::string predicate;            // WHERE条件（先用string占位）
};
