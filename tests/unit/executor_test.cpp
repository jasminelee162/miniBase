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

    /* 2. 插入多行数据以便测试排序 */
    {
        std::vector<std::vector<std::string>> vals = {
            {"201", "Alice Smith", "Math", "5"},
            {"202", "Bob Johnson", "English", "8"},
            {"203", "Charlie Lee", "Math", "3"},
            {"204", "Diana King", "English", "7"},
            {"205", "Eve Adams", "Math", "10"}};

        for (auto &v : vals)
        {
            PlanNode ins;
            ins.type = PlanType::Insert;
            ins.table_name = "teachers";
            ins.columns = {"teacher_id", "full_name", "subject", "experience"};
            ins.values = {v};
            exec.execute(&ins);
        }
    }

    /* 3. OrderBy 测试 */
    auto orderByTest = [&](const std::string &col, bool desc)
    {
        std::cout << "\n== OrderBy " << col << (desc ? " DESC" : " ASC") << " ==\n";

        auto scan = std::make_unique<PlanNode>();
        scan->type = PlanType::SeqScan;
        scan->table_name = "teachers";

        PlanNode ob;
        ob.type = PlanType::OrderBy;
        ob.order_by_cols = {col};
        ob.order_by_desc = desc;
        ob.children.push_back(std::move(scan));

        printResult(exec.execute(&ob));
    };

    orderByTest("experience", false); // 升序
    orderByTest("full_name", true);   // 降序
    orderByTest("teacher_id", false); // 升序

    return 0;
}
