#pragma once

#include "../../engine/executor/executor.h"
#include "../../catalog/catalog.h"
#include <string>

namespace minidb
{

    /**
     * @brief SQLImporter
     * 解析简单 SQL 文件（仅支持 CREATE TABLE 和 INSERT INTO）
     * 并调用 Executor 执行，同时维护 Catalog 与数据页结构。
     */
    class SQLImporter
    {
    public:
        /**
         * @param exec Executor 指针，用于执行 PlanNode
         * @param catalog Catalog 指针，用于获取表结构和写入目录页
         */
        SQLImporter(Executor *exec, Catalog *catalog);

        /**
         * @brief 从 SQL 文件导入
         * @param filename SQL 文件路径
         * @return 成功返回 true，失败返回 false
         */
        bool ImportSQLFile(const std::string &filename);

        /**
         * @brief 从 SQL 内容直接导入
         * @param content SQL 内容字符串
         * @return 成功返回 true，失败返回 false
         */
        bool ImportSQLContent(const std::string &content);

        // ✅ 确保这个函数在 public 里
        bool ExecuteSQLFile(const std::string &filename);

    private:
        Executor *exec_;
        Catalog *catalog_;

        /**
         * @brief 执行单条 SQL 语句
         */
        bool ExecuteSQL(const std::string &sql);

        /**
         * @brief 处理 CREATE TABLE 语句
         */
        bool HandleCreate(const std::string &sql);

        /**
         * @brief 处理 INSERT INTO 语句
         */
        bool HandleInsert(const std::string &sql);
    };

} // namespace minidb
