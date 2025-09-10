<<<<<<< HEAD
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
=======
#ifndef MINIBASE_CATALOG_H
#define MINIBASE_CATALOG_H

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <stdexcept>

// 列信息类
class ColumnInfo {
public:
    enum class DataType {
        INT,
        VARCHAR
    };
    
    ColumnInfo(const std::string& name, DataType type, int index)
        : name(name), type(type), index(index) {}
    
    const std::string& getName() const { return name; }
    DataType getType() const { return type; }
    int getIndex() const { return index; }
    
    static std::string dataTypeToString(DataType type) {
        switch (type) {
            case DataType::INT: return "INT";
            case DataType::VARCHAR: return "VARCHAR";
            default: return "UNKNOWN";
        }
    }
    
    static DataType stringToDataType(const std::string& typeStr) {
        if (typeStr == "INT") return DataType::INT;
        if (typeStr == "VARCHAR") return DataType::VARCHAR;
        throw std::invalid_argument("Unknown data type: " + typeStr);
    }
    
private:
    std::string name;
    DataType type;
    int index;  // 列在表中的位置索引
};

// 表信息类
class TableInfo {
public:
    TableInfo(const std::string& name) : name(name) {}
    
    const std::string& getName() const { return name; }
    
    // 添加列
    void addColumn(const std::string& name, ColumnInfo::DataType type) {
        columns.emplace_back(name, type, static_cast<int>(columns.size()));
        columnMap[name] = columns.size() - 1;
    }
    
    // 获取所有列
    const std::vector<ColumnInfo>& getColumns() const { return columns; }
    
    // 根据名称获取列
    const ColumnInfo* getColumn(const std::string& name) const {
        auto it = columnMap.find(name);
        if (it == columnMap.end()) {
            return nullptr;
        }
        return &columns[it->second];
    }
    
    // 检查列是否存在
    bool hasColumn(const std::string& name) const {
        return columnMap.find(name) != columnMap.end();
    }
    
    // 获取列数
    size_t getColumnCount() const { return columns.size(); }
    
private:
    std::string name;
    std::vector<ColumnInfo> columns;
    std::unordered_map<std::string, size_t> columnMap;  // 列名到索引的映射
};

// 目录类（单例模式）
class Catalog {
public:
    // 获取单例实例
    static Catalog& getInstance() {
        static Catalog instance;
        return instance;
    }
    
    // 创建表
    void createTable(const std::string& name, const std::vector<std::pair<std::string, ColumnInfo::DataType>>& columns) {
        if (hasTable(name)) {
            throw std::runtime_error("Table already exists: " + name);
        }
        
        auto tableInfo = std::make_shared<TableInfo>(name);
        for (const auto& col : columns) {
            tableInfo->addColumn(col.first, col.second);
        }
        
        tables[name] = tableInfo;
    }
    
    // 获取表信息
    std::shared_ptr<TableInfo> getTable(const std::string& name) const {
        auto it = tables.find(name);
        if (it == tables.end()) {
            return nullptr;
        }
        return it->second;
    }
    
    // 检查表是否存在
    bool hasTable(const std::string& name) const {
        return tables.find(name) != tables.end();
    }
    
    // 获取所有表名
    std::vector<std::string> getAllTableNames() const {
        std::vector<std::string> names;
        for (const auto& pair : tables) {
            names.push_back(pair.first);
        }
        return names;
    }
    
    // 清空目录（用于测试）
    void clear() {
        tables.clear();
    }
    
private:
    // 私有构造函数（单例模式）
    Catalog() {}
    
    // 禁止拷贝和赋值（单例模式）
    Catalog(const Catalog&) = delete;
    Catalog& operator=(const Catalog&) = delete;
    
    std::unordered_map<std::string, std::shared_ptr<TableInfo>> tables;
};

#endif // MINIBASE_CATALOG_H
>>>>>>> sql_compiler
