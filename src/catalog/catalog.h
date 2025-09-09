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
        std::string type; // 先用 string 占位（未来可换成枚举类型）
    };

    // 表定义
    struct TableSchema
    {
        std::string table_name;
        std::vector<Column> columns;
    };

    class Catalog
    {
    public:
        Catalog(const std::string &catalog_file);

        // 建表
        void CreateTable(const std::string &table_name, const std::vector<std::string> &columns);

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
    };

} // namespace minidb
