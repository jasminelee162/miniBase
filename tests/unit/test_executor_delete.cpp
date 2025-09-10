// tests/unit/test_executor_delete.cpp
#include <iostream>
#include <memory>
#include "../../src/engine/executor/executor.h"
#include "../../src/storage/page/disk_manager.h"

using namespace minidb;

int main()
{
    std::cout << "Running Executor Delete Test..." << std::endl;

    auto dm = std::make_shared<DiskManager>("data/test_delete.db");
    Executor exec(dm);

    // 插入一行
    PlanNode ins;
    ins.type = PlanType::Insert;
    ins.table_name = "users";
    ins.columns = {"1", "Alice", "20"};
    exec.execute(&ins);

    // 确认插入
    PlanNode scan;
    scan.type = PlanType::SeqScan;
    scan.table_name = "users";
    exec.execute(&scan);

    // 删除匹配 Alice 的行（用简单的 substring 匹配）
    PlanNode del;
    del.type = PlanType::Delete;
    del.table_name = "users";
    del.predicate = "Alice";
    exec.execute(&del);

    // 再次扫描，应无行
    exec.execute(&scan);

    std::cout << "Executor Delete Test finished." << std::endl;
    return 0;
}
