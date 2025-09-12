#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <mutex> // 这里引入 std::recursive_mutex

namespace minidb
{

    // 列定义
    struct Column
    {
        std::string name;
        std::string type; // INT, VARCHAR 等
        int length;       // 对于 VARCHAR/CHAR 有意义，其他类型可设为 -1
    };

    // 表定义
    struct TableSchema
    {
        std::string table_name;
        std::vector<Column> columns;

        int getColumnIndex(const std::string &col_name) const
        {
            for (size_t i = 0; i < columns.size(); i++)
            {
                if (columns[i].name == col_name)
                    return static_cast<int>(i);
            }
            return -1;
        }
    };

    class Catalog
    {
    public:
        Catalog(const std::string &catalog_file);

        // 可选：静默模式开关
        void SetQuiet(bool q) { quiet_ = q; }

        // 建表
        // void CreateTable(const std::string &table_name, const std::vector<std::string> &columns);
        void CreateTable(const std::string &table_name,
                         const std::vector<Column> &columns);
        std::vector<std::string> GetTableColumns(const std::string &table_name);

        // 查询表
        bool HasTable(const std::string &table_name) const;
        TableSchema GetTable(const std::string &table_name) const;

        // 持久化 & 加载
        void Load();
        void Save();

    private:
        std::string catalog_file_;
        std::unordered_map<std::string, TableSchema> tables_;
        mutable std::recursive_mutex latch_; // ← 改成递归锁
        bool quiet_ { false };
        bool loaded_ { false };
    };

}