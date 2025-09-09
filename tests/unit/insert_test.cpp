#include <iostream>
#include <string>
#include "../../src/engine/executor/executor.h"
#include "../../src/engine/operators/plan_node.h"
#include "storage/page/disk_manager.h"
#include "util/config.h"

using namespace minidb;

int main()
{
    std::cout << "==== Insert Test Start ====" << std::endl;

    // 1. 初始化 DiskManager
    DiskManager disk_manager("test.db");

    // 2. 初始化 Executor（带 disk_manager）
    Executor executor(&disk_manager);

    // 3. 构造 Insert PlanNode
    PlanNode node;
    node.type = PlanType::Insert;
    node.table_name = "test_table";
    node.columns = {"1", "Alice", "CS"};

    // 4. 执行 Insert
    executor.execute(&node);

    // 5. 读出写入的第一个页面
    char buf[PAGE_SIZE];
    auto status = disk_manager.ReadPage(1, buf);
    if (status != Status::OK)
    {
        std::cerr << "[Test] ReadPage 失败" << std::endl;
        return -1;
    }

    // 6. 打印前 200 字节内容（可见写入的数据）
    std::cout << "[Test] Page 1 内容（前200字节）:" << std::endl;
    for (int i = 0; i < 200; i++)
    {
        if (buf[i] == '\0')
            std::cout << '.';
        else
            std::cout << buf[i];
    }
    std::cout << std::endl;

    std::cout << "==== Insert Test End ====" << std::endl;
    return 0;
}
