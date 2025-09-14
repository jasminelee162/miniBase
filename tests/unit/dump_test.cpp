#include "../../src/engine/executor/executor.h"
#include "../../src/engine/operators/plan_node.h"
#include "../../src/catalog/catalog.h"
#include "../../src/tools/sql_dump/sql_dumper.h"
#include <iostream>
#include <memory>
#ifdef _WIN32
#include <windows.h>
#endif

int main()
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    // 初始化存储和执行器
    auto engine = std::make_shared<minidb::StorageEngine>("school.db", 10);
    auto catalog = std::make_shared<minidb::Catalog>(engine.get());
    minidb::Executor exec(engine);
    exec.SetCatalog(catalog);

    /* 1. 建表 */
    {
        PlanNode ct;
        ct.type = PlanType::CreateTable;
        ct.table_name = "students";
        ct.table_columns = {
            {"id", "INT", -1},
            {"name", "VARCHAR", 50},
            {"age", "INT", -1}};
        exec.execute(&ct);
    }

    /* 2. 插入数据 */
    {
        std::vector<std::vector<std::string>> vals = {
            {"1", "Alice", "20"},
            {"2", "Bob", "22"},
            {"3", "Charlie", "19"}};

        for (auto &v : vals)
        {
            PlanNode ins;
            ins.type = PlanType::Insert;
            ins.table_name = "students";
            ins.columns = {"id", "name", "age"};
            ins.values = {v};
            exec.execute(&ins);
        }
    }

    /* 3. 执行转储 */
    {
        minidb::SQLDumper dumper(catalog.get(), engine.get());

        // 仅导出结构
        dumper.DumpToFile("dump_structure.sql", minidb::DumpOption::StructureOnly);
        std::cout << "已生成 dump_structure.sql (仅结构)\n";

        // 导出结构 + 数据
        dumper.DumpToFile("dump_all.sql", minidb::DumpOption::StructureAndData);
        std::cout << "已生成 dump_all.sql (结构+数据)\n";
    }

    return 0;
}
