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

    // 新增：序列化
    void Serialize(std::vector<char> &out_buf, const minidb::TableSchema &schema) const
    {
        out_buf.clear();
        // 先存列数
        int32_t col_count = static_cast<int32_t>(columns.size());
        out_buf.resize(sizeof(int32_t));
        std::memcpy(out_buf.data(), &col_count, sizeof(int32_t));

        for (size_t i = 0; i < schema.columns.size(); i++)
        {
            const auto &col_schema = schema.columns[i];
            std::string val = "";
            if (i < columns.size())
                val = columns[i].value;

            if (col_schema.type == "INT")
            {
                int32_t num = 0;
                if (!val.empty())
                    num = std::stoi(val);
                size_t old_size = out_buf.size();
                out_buf.resize(old_size + sizeof(int32_t));
                std::memcpy(out_buf.data() + old_size, &num, sizeof(int32_t));
            }
            else if (col_schema.type == "DOUBLE")
            {
                double num = 0.0;
                if (!val.empty())
                    num = std::stod(val);
                size_t old_size = out_buf.size();
                out_buf.resize(old_size + sizeof(double));
                std::memcpy(out_buf.data() + old_size, &num, sizeof(double));
            }
            else // VARCHAR / CHAR
            {
                size_t field_size = col_schema.length > 0 ? col_schema.length : 64;
                if (val.size() > field_size)
                    val = val.substr(0, field_size);
                size_t old_size = out_buf.size();
                out_buf.resize(old_size + field_size);
                std::memset(out_buf.data() + old_size, 0, field_size);
                std::memcpy(out_buf.data() + old_size, val.data(), val.size());
            }
        }
    }

    // ===== 新增 Deserialize =====
    static Row Deserialize(const unsigned char *data, uint16_t size, const minidb::TableSchema &schema)
    {
        Row row;
        if (!data || size < sizeof(int32_t))
            return row;

        size_t offset = 0;
        int32_t col_count = 0;
        std::memcpy(&col_count, data + offset, sizeof(int32_t));
        offset += sizeof(int32_t);

        for (size_t i = 0; i < schema.columns.size(); ++i)
        {
            const auto &col_schema = schema.columns[i];
            std::string val;

            if (col_schema.type == "INT")
            {
                if (offset + sizeof(int32_t) > size)
                    break;
                int32_t num = 0;
                std::memcpy(&num, data + offset, sizeof(int32_t));
                val = std::to_string(num);
                offset += sizeof(int32_t);
            }
            else if (col_schema.type == "DOUBLE")
            {
                if (offset + sizeof(double) > size)
                    break;
                double num = 0.0;
                std::memcpy(&num, data + offset, sizeof(double));
                val = std::to_string(num);
                offset += sizeof(double);
            }
            else // VARCHAR / CHAR
            {
                size_t field_size = col_schema.length > 0 ? col_schema.length : 64;
                if (offset + field_size > size)
                    break;
                val.assign(reinterpret_cast<const char *>(data + offset), field_size);
                // 去掉末尾填充的 '\0'
                val.erase(val.find_last_not_of('\0') + 1);
                offset += field_size;
            }

            row.columns.emplace_back(col_schema.name, val);
        }

        return row;
    }
};