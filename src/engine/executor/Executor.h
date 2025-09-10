#pragma once

#include <memory>
#include <vector>
#include <string>
#include "../../catalog/catalog.h"        // 确保能 include 到你发的 Catalog
#include "../operators/plan_node.h"       // PlanNode
#include "../../storage/storage_engine.h" // ✅ 改这里
// #include "../../storage/page/disk_manager.h"
#include "../../storage/page/page.h" // Page
#include "../operators/row.h"        // Row, ColumnValue
#include "../../util/logger.h"
#include <iostream>

namespace minidb
{
    class Executor
    {
    public:
        Executor(std::shared_ptr<StorageEngine> se) : storage_engine_(se) {} // ✅ 改构造函数

        void execute(PlanNode *node);
        void SetStorageEngine(std::shared_ptr<StorageEngine> se) { storage_engine_ = se; }
        Executor() = default;

        // 扫描算子
        // std::vector<Row> SeqScan(page_id_t page_id); // 单页
        // std::vector<Row> SeqScanAll(); // 所有页
        std::vector<Row> SeqScanAll(const std::string &table_name);
        std::vector<Row> SeqScan(page_id_t page_id, const std::vector<std::string> &columns);

        // 其他算子
        std::vector<Row> Filter(const std::vector<Row> &rows, const std::string &predicate);
        std::vector<Row> Project(const std::vector<Row> &rows, const std::vector<std::string> &cols);

        void Update(const PlanNode &plan); // 有没有问题
        void executeSelect(const PlanNode &plan);

        void SetCatalog(std::shared_ptr<minidb::Catalog> catalog)
        {
            catalog_ = catalog;
            std::cout << "[Executor] SetCatalog called, catalog ptr=" << catalog_.get() << std::endl;
        }

    private:
        std::shared_ptr<StorageEngine> storage_engine_;
        static Logger logger;
        std::shared_ptr<Catalog> catalog_; // 新增
    };

} // namespace minidb
