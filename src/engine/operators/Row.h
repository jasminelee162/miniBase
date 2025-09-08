/**
 * 存储列名和对应值
 */
#pragma once
#include <string>
#include <vector>

struct ColumnValue
{
    std::string col_name;
    std::string value; // 先用string，后续可扩展类型
};

struct Row
{
    std::vector<ColumnValue> columns;
};
