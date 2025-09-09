#include "miniCLI.h"
#include "../frontend/parser/SQLParser.h"
#include "../engine/executor/Executor.h"
#include <iostream>

void MiniCLI::run()
{
    std::string sql;
    Executor executor;

    while (true)
    {
        std::cout << "miniDB> ";
        std::getline(std::cin, sql);
        if (sql == "exit")
            break;

        PlanNode *plan = SQLParser::parse(sql); // 解析 SQL
        if (plan)
        {
            executor.execute(plan); // 执行计划
        }
    }
}
