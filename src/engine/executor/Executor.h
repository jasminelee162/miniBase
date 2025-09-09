#pragma once

#include <memory>
#include <vector>
#include <string>
#include "../operators/plan_node.h" // PlanNode
#include "../../storage/page/disk_manager.h"
#include "../../storage/page/page.h" // Page
#include "../operators/row.h"        // Row, ColumnValue
#include "../../util/logger.h"

namespace minidb
{

    class Executor
    {
    public:
        explicit Executor(DiskManager *dm) : disk_manager_(dm) {}

        void execute(PlanNode *node);

    private:
        DiskManager *disk_manager_{nullptr}; // 用来写页的
        Logger logger{"minidb.log"};
    };

} // namespace minidb
