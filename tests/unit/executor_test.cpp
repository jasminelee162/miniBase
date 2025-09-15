#include "../../src/catalog/catalog.h"
#include "../../src/engine/executor/executor.h"
#include "../../src/engine/operators/plan_node.h"
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

    std::cout << "???????????????";
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

    /* 2. SHOW TABLES (删除前) */
    {
        PlanNode show;
        show.type = PlanType::ShowTables;
        std::cout << "\n== SHOW TABLES (before drop) ==\n";
        printResult(exec.execute(&show));
    }

    /* 3. CREATE PROCEDURE test_proc */
    {
        PlanNode proc;
        proc.type = PlanType::CreateProcedure;
        proc.proc_name = "test_proc";
        proc.proc_params = {"teacher_name"};
        proc.proc_body = "INSERT INTO teachers (teacher_id, full_name, subject, experience) VALUES (1, ?, 'Math', 5);";
        std::cout << "\n== CREATE PROCEDURE test_proc ==\n";
        exec.execute(&proc);
    }

    /* 4. CALL PROCEDURE test_proc */
    {
        PlanNode call;
        call.type = PlanType::CallProcedure;
        call.proc_name = "test_proc";
        call.proc_args = {"John Smith"};
        std::cout << "\n== CALL PROCEDURE test_proc ==\n";
        exec.execute(&call);
    }

    /* 6. DROP TABLE students */
    {
        PlanNode drop;
        drop.type = PlanType::Drop;
        drop.table_name = "students";
        std::cout << "\n== DROP TABLE students ==\n";
        exec.execute(&drop);
    }

    /* 7. SHOW TABLES (删除后) */
    {
        PlanNode show;
        show.type = PlanType::ShowTables;
        std::cout << "\n== SHOW TABLES (after drop) ==\n";
        printResult(exec.execute(&show));
    }

    return 0;
}
