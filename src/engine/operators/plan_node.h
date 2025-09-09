/**
 * 定义执行计划节点类型和结构（增强版）
 */
#pragma once
#include <string>
#include <vector>
#include <memory>

enum class PlanType
{
    SeqScan,
    Filter,
    Project,
    CreateTable,
    Insert,
    Delete
};

// 表字段定义（用于 CREATE TABLE）
struct ColumnDef
{
    std::string name;
    std::string type; // 简化：int / varchar 等
};

struct PlanNode
{
    PlanType type;

    // 子节点（如 Project -> SeqScan）
    std::vector<std::unique_ptr<PlanNode>> children;

    // 通用信息
    std::string table_name;           // 表名
    std::vector<std::string> columns; // SELECT 列
    std::string predicate;            // WHERE 条件

    // INSERT 相关
    std::vector<std::string> values; // 插入的值

    // CREATE TABLE 相关
    std::vector<ColumnDef> schema; // 表模式
};
