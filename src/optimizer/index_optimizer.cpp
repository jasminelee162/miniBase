#include "optimizer/index_optimizer.h"
#include <cmath>
#include <iostream>
#include "../util/logger.h"

namespace minidb
{

    IndexOptimizer::IndexOptimizer(Catalog *catalog)
        : catalog_(catalog), engine_(nullptr)
    {
        if (catalog_)
        {
            // 让 optimizer 能用到 catalog 里的 StorageEngine
            engine_ = catalog_->GetStorageEngine();
        }
    }

    double IndexOptimizer::EstimateCost(BPlusTree *index, int low, int high, size_t table_rows)
    {
        if (!index)
            return static_cast<double>(table_rows); // 没索引 -> 全表扫描

        size_t range_size = (high >= low) ? (high - low + 1) : 1;
        size_t n = index->GetKeyCount();
        if (n == 0)
            return static_cast<double>(table_rows);

        double log_cost = std::log2(static_cast<double>(n));
        return log_cost + static_cast<double>(range_size);
    }

    BPlusTree *IndexOptimizer::ChooseBestIndex(const std::string &table_name,
                                               int low, int high)
    {
        if (!catalog_ || !catalog_->HasTable(table_name))
        {
            global_log_warn(std::string("[IndexOptimizer] 表不存在: ") + table_name);
            return nullptr;
        }

        TableSchema schema = catalog_->GetTable(table_name);
        size_t table_rows = 1000; // 默认行数，后面可改为统计值

        // 从 Catalog 拿到该表的索引定义
        auto index_defs = catalog_->GetTableIndexes(table_name);

        double best_cost = std::numeric_limits<double>::max();
        BPlusTree *best_index = nullptr;

        for (const auto &idx : index_defs)
        {
            if (idx.type != "BPLUS")
                continue; // 暂时只支持 B+树

            // 用 IndexSchema 初始化一个 BPlusTree（通过 root_page_id）
            auto *bptree = new BPlusTree(engine_);
            bptree->SetRoot(idx.root_page_id);

            double cost = EstimateCost(bptree, low, high, table_rows);
            if (cost < best_cost)
            {
                best_cost = cost;
                best_index = bptree;
            }
        }

        if (best_index)
        {
            global_log_debug(std::string("[IndexOptimizer] 选择索引 (root_page_id=") + std::to_string(best_index->GetRoot()) + ") 代价=" + std::to_string(best_cost));
        }
        else
        {
            global_log_info("[IndexOptimizer] 未找到合适索引，走全表扫描");
        }

        return best_index;
    }

    void IndexOptimizer::RebuildIndex(const std::string &table_name)
    {
        if (!catalog_ || !catalog_->HasTable(table_name))
        {
            global_log_warn(std::string("[IndexOptimizer] 表不存在: ") + table_name);
            return;
        }

        auto index_defs = catalog_->GetTableIndexes(table_name);
        for (const auto &idx : index_defs)
        {
            global_log_info(std::string("[IndexOptimizer] 正在重建索引 ") + idx.index_name + " (root_page_id=" + std::to_string(idx.root_page_id) + ") ...");

            // TODO: 遍历表数据，重新插入到一个新的 B+树里
        }
    }

}
