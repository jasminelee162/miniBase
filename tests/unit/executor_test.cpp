#include "../../src/engine/executor/executor.h"
#include "../../src/engine/operators/plan_node.h"
#include "../../src/catalog/catalog.h"
#include <iostream>
#include <memory>
#ifdef _WIN32
#include <windows.h>
#endif

void printResult(const std::vector<Row> &rows)
{
    if (rows.empty())
    {
        std::cout << "(no rows)\n";
        return;
    }
    for (const auto &r : rows)
        std::cout << r.toString() << '\n';
}

int main()
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    auto engine = std::make_shared<minidb::StorageEngine>("school.db", 10);
    minidb::Executor exec(engine);
    auto catalog = std::make_shared<minidb::Catalog>(engine.get());
    exec.SetCatalog(catalog);

    /* 1. 建表 */
    {
        PlanNode ct;
        ct.type = PlanType::CreateTable;
        ct.table_name = "teachers";
        ct.table_columns = {
            {"teacher_id", "INT", -1},
            {"full_name", "VARCHAR", 100},
            {"subject", "VARCHAR", 50},
            {"experience", "INT", -1}};
        exec.execute(&ct);
    }

    {
        PlanNode ct;
        ct.type = PlanType::CreateTable;
        ct.table_name = "students";
        ct.table_columns = {
            {"student_id", "INT", -1},
            {"name", "VARCHAR", 100},
            {"age", "INT", -1}};
        exec.execute(&ct);
    }

    /* 2. 测试 SHOW TABLES */
    {
        PlanNode show;
        show.type = PlanType::ShowTables;
        std::cout << "\n== SHOW TABLES ==\n";
        printResult(exec.execute(&show));
    }

    return 0;
}
