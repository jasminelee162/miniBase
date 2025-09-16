#pragma once

#include <memory>
#include <vector>
#include <string>
#include "../../catalog/catalog.h"        // 确保能 include 到你发的 Catalog
#include "../operators/plan_node.h"       // PlanNode
#include "../../storage/storage_engine.h" // ✅ 改这里
#include "../../storage/page/page.h"      // Page
#include "../operators/row.h"             // Row, ColumnValue
#include "../../util/logger.h"
#include "../../optimizer/index_optimizer.h" // 新增：索引优化器
#include "../../auth/permission_checker.h"
#include "../../auth/auth_service.h" // AuthService

#include <iostream>

namespace minidb
{
    class Executor
    {
    public:
        Executor(std::shared_ptr<StorageEngine> se) : storage_engine_(se) {}
        std::vector<std::string> expandWildcardColumns(const PlanNode *node);
        // ✅ 改构造函数
        std::vector<Row> execute(PlanNode *node);
        Executor(StorageEngine *storage_engine, Catalog *catalog, AuthService *auth_service); // 构造函数
        std::string parseColumnFromBuffer(const void *data, size_t &offset, const std::string &col_name, const std::string &table_name);
        Row parseRowFromPage(Page *page, const std::vector<std::string> &columns, const std::string &table_name);

        void SetStorageEngine(std::shared_ptr<StorageEngine> se) { storage_engine_ = se; }
        Executor() = default;

        // 扫描算子
        std::vector<Row> SeqScanAll(const std::string &table_name);
        std::vector<Row> SeqScan(Page *page, const minidb::TableSchema &schema);

        // 其他算子
        std::vector<Row> Filter(const std::vector<Row> &rows, const std::string &predicate);
        std::vector<Row> Project(const std::vector<Row> &rows, const std::vector<std::string> &cols);

        void Update(const PlanNode &plan); // 有没有问题
        void executeSelect(const PlanNode &plan);

        void SetCatalog(std::shared_ptr<minidb::Catalog> catalog)
        {
            catalog_ = catalog;
        }

        // ✅ 新增 Getter
        std::shared_ptr<StorageEngine> GetStorageEngine() const
        {
            return storage_engine_;
        }

        Executor(Catalog *catalog, PermissionChecker *checker)
            : catalog_(catalog), permissionChecker_(checker) {}

    private:
        std::shared_ptr<StorageEngine> storage_engine_;
        static Logger logger;
        std::shared_ptr<Catalog> catalog_; // 新增

        PermissionChecker *permissionChecker_; // 权限检查器
        std::unique_ptr<IndexOptimizer> optimizer_;
        AuthService *auth_service_; // ✅ 新增
    };

} // namespace minidb
