#pragma once
#include "../storage/index/bplus_tree.h"
#include "../catalog/catalog.h"
#include <limits>
#include <string>
#include <vector>

namespace minidb
{

    class IndexOptimizer
    {
    public:
        explicit IndexOptimizer(Catalog *catalog);

        // 估算一次查询代价
        double EstimateCost(BPlusTree *index, int low, int high, size_t table_rows);

        // 根据谓词选择最优索引
        BPlusTree *ChooseBestIndex(const std::string &table_name,
                                   int low, int high);

        // 重建索引（示例逻辑）
        void RebuildIndex(const std::string &table_name);

    private:
        Catalog *catalog_;
        StorageEngine *engine_; // 需要用来构造/加载 B+Tree
    };

}
