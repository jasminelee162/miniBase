#include "../../src/engine/executor/executor.h"
#include "storage/page/disk_manager.h"
#include <iostream>
#include <memory>

using namespace minidb;

int main()
{
    std::cout << "Running Executor Insert Test..." << std::endl;

    // 1. 创建 DiskManager
    auto dm = std::make_shared<DiskManager>("test_db.dat");

    // 2. 创建 Executor
    Executor executor(dm);

    // 3. 手动构造 PlanNode（模拟解析器输出）
    PlanNode insert_node;
    insert_node.type = PlanType::Insert;
    insert_node.table_name = "users";
    insert_node.columns = {"1", "Alice", "20"};

    // 4. 调用 execute 执行 Insert
    executor.execute(&insert_node);

    // 5. 再执行一次 SeqScan 查看写入效果
    PlanNode scan_node;
    scan_node.type = PlanType::SeqScan;
    scan_node.table_name = "users";

    executor.execute(&scan_node);

    std::cout << "Executor Insert Test finished." << std::endl;
    return 0;
}
