/**
 * 搭建命令行框架，可以先测试输入输出
 */

#include "MiniCLI.h"
#include <iostream>

void MiniCLI::run()
{
    std::string sql;
    while (true)
    {
        std::cout << "miniDB> ";
        std::getline(std::cin, sql);
        if (sql == "exit")
            break;
        std::cout << "[模拟] 接收到 SQL: " << sql << std::endl;
        // TODO: 未来这里可以调用编译器生成的 PlanNode 并执行
    }
}
