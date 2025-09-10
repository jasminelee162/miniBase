/**
 * 存储列名和对应值
 * Row 与 ColumnValue 定义
 */
#pragma once
#include <string>
#include <vector>
#include <sstream>

struct ColumnValue
{
    std::string col_name;
    std::string value; // 先用 string，后续可扩展类型

    ColumnValue() = default;
    ColumnValue(const std::string &name, const std::string &val)
        : col_name(name), value(val) {}
};

struct Row
{
    std::vector<ColumnValue> columns;
    std::vector<std::string> values;

    Row() = default;

    // 支持通过 initializer_list 构造
    Row(std::initializer_list<ColumnValue> cols) : columns(cols) {}

    // 根据列名获取值
    std::string getValue(const std::string &col) const
    {
        for (const auto &c : columns)
        {
            if (c.col_name == col)
            {
                return c.value;
            }
        }
        return ""; // 未找到返回空
    }

    // 转换为字符串，调试用
    std::string toString() const
    {
        std::ostringstream oss;
        oss << "{";
        for (size_t i = 0; i < columns.size(); i++)
        {
            oss << columns[i].col_name << ": " << columns[i].value;
            if (i < columns.size() - 1)
                oss << ", ";
        }
        oss << "}";
        return oss.str();
    }

    void setValue(const std::string &col, const std::string &val)
    {
        for (auto &c : columns)
        {
            if (c.col_name == col)
            {
                c.value = val;
                return;
            }
        }
        // 如果没找到列，可以选择新增一列
        columns.emplace_back(col, val);
    }
};