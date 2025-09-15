#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <sstream>
#include "../frontend/translator/json_to_plan.h"
#include "../storage/storage_engine.h" // 提供 page_id_t, Page 等类型
#include "../util/json.hpp"
using json = nlohmann::json;

namespace minidb
{

    // 列定义
    struct Column
    {
        std::string name;
        std::string type; // INT, VARCHAR, ...
        int length;       // VARCHAR/CHAR 用，其他类型可 -1
    };

    // 索引定义
    struct IndexSchema
    {
        std::string index_name;                  // 索引名
        std::string table_name;                  // 所属表
        std::vector<std::string> cols;           // 索引列
        std::string type;                        // BPLUS / HASH
        page_id_t root_page_id{INVALID_PAGE_ID}; // 索引入口页（B+ 树 root）
    };

    // 存储过程定义
    struct ProcedureDef
    {
        std::string name;
        std::vector<std::string> params; // 形参
        std::string body;                // SQL 体
    };

    // 表定义
    struct TableSchema
    {
        std::string table_name;
        std::vector<Column> columns;

        // 新增：记录表的首个数据页
        page_id_t first_page_id{INVALID_PAGE_ID};

        // 新增：表所有者信息
        std::string owner; // 表创建者
        time_t created_at; // 创建时间

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

    inline void from_json(const json &j, Column &c)
    {
        j.at("name").get_to(c.name);
        j.at("type").get_to(c.type);
        j.at("length").get_to(c.length);
    }

    class Catalog
    {
    public:
        Catalog() = default;
        explicit Catalog(StorageEngine *engine); // 可通过构造注入 engine

        // 允许后续在任意时刻注入 StorageEngine
        void SetStorageEngine(StorageEngine *engine);

        // ===== 表管理 =====
        void CreateTable(const std::string &table_name, const std::vector<Column> &columns);
        void CreateTable(const std::string &table_name, const std::vector<Column> &columns, const std::string &owner);

        void LoadFromStorage();

        std::vector<std::string> GetTableColumns(const std::string &table_name);
        bool HasTable(const std::string &table_name) const;
        TableSchema GetTable(const std::string &table_name) const;

        // 表所有者相关接口
        std::string GetTableOwner(const std::string &table_name) const;
        bool IsTableOwner(const std::string &table_name, const std::string &username) const;
        std::vector<std::string> GetTablesByOwner(const std::string &username) const;

        // 获取所有表名
        std::vector<std::string> GetAllTableNames() const;

        void SaveToStorage();

        // ===== 索引管理 =====
        // 注意：CreateIndex 不再需要外部传入 engine（用内部 storage_engine_）
        void CreateIndex(const std::string &index_name,
                         const std::string &table_name,
                         const std::vector<std::string> &cols,
                         const std::string &type);
        bool HasIndex(const std::string &index_name) const;
        IndexSchema GetIndex(const std::string &index_name) const;
        std::vector<IndexSchema> GetTableIndexes(const std::string &table_name) const;

        std::string FindIndexByColumn(const std::string &table_name, const std::string &col) const;

        std::vector<std::string> GetAllTables() const; // 导出中使用

        // ===== 删除接口 =====
        void DropTable(const std::string &table_name);
        void DropIndex(const std::string &index_name);

        std::unordered_map<std::string, ProcedureDef> procedures_;
        void CreateProcedure(const ProcedureDef &proc)
        {
            procedures_[proc.name] = proc;
        }

        bool HasProcedure(const std::string &name) const
        {
            return procedures_.find(name) != procedures_.end();
        }

        const ProcedureDef &GetProcedure(const std::string &name) const
        {
            return procedures_.at(name);
        }

    private:
        StorageEngine *storage_engine_{nullptr}; // 如果通过构造或 Set 注入则使用
        std::unordered_map<std::string, TableSchema> tables_;
        std::unordered_map<std::string, IndexSchema> indexes_;
        mutable std::recursive_mutex latch_;
    };

} // namespace minidb