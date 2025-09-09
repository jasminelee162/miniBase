#include <iostream>
#include <memory>
#include "../../src/engine/executor/executor.h"
#include "../src/storage/page/disk_manager.h"
#include "../src/util/logger.h"

using namespace minidb;

int main()
{
    std::cout << "Running SeqScan Test..." << std::endl;

    // 初始化 DiskManager
    DiskManager dm("data/test_seq_scan.db"); // 非 shared_ptr
    Executor executor(&dm);
    // 1. 插入一行数据
    PlanNode insert_node;
    insert_node.type = PlanType::Insert;
    insert_node.table_name = "users";
    insert_node.columns = {"Alice", "25", "Engineer"};

    executor.execute(&insert_node);

    // 2. 扫描表（读取刚写入的页面）
    PlanNode scan_node;
    scan_node.type = PlanType::SeqScan;
    scan_node.table_name = "users";

    executor.execute(&scan_node);

    std::cout << "SeqScan Test finished." << std::endl;
    return 0;
}
