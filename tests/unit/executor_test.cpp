#include "../../src/engine/executor/executor.h"
#include "../../src/engine/operators/plan_node.h"
#include "../../src/catalog/catalog.h"
#include "../../src/transaction/transaction_manager.h" // 假设你写的事务管理头
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

    // --- 初始化 ---
    auto engine = std::make_shared<minidb::StorageEngine>("school.db", 10);
    auto txn_mgr = std::make_shared<minidb::TransactionManager>(engine); // 传入 engine
    minidb::Executor exec(engine);
    auto catalog = std::make_shared<minidb::Catalog>(engine.get());
    exec.SetCatalog(catalog);
    exec.SetTransactionManager(txn_mgr); // Executor 内部使用 txn_mgr_

    // =========================
    // 1. 建表
    // =========================
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

    // =========================
    // 2. 插入测试
    // =========================
    {
        PlanNode ins;
        ins.type = PlanType::Insert;
        ins.table_name = "students";
        ins.columns = {"student_id", "name", "age"};
        ins.values = {
            {"1", "Alice", "20"},
            {"2", "Bob", "21"},
            {"3", "Charlie", "22"}};
        exec.execute(&ins);
        std::cout << "[Test] Inserted 3 students\n";
    }

    // =========================
    // 3. 更新测试
    // =========================
    {
        PlanNode upd;
        upd.type = PlanType::Update;
        upd.table_name = "students";
        upd.predicate = "student_id=2"; // 更新 Bob
        upd.set_values = {{"age", "23"}};
        exec.execute(&upd);
        std::cout << "[Test] Updated student_id=2 age to 23\n";
    }

    // =========================
    // 4. 删除测试
    // =========================
    {
        PlanNode del;
        del.type = PlanType::Delete;
        del.table_name = "students";
        del.predicate = "student_id=1"; // 删除 Alice
        exec.execute(&del);
        std::cout << "[Test] Deleted student_id=1\n";
    }

    // =========================
    // 5. SHOW TABLES
    // =========================
    {
        PlanNode show;
        show.type = PlanType::ShowTables;
        std::cout << "\n== SHOW TABLES ==\n";
        printResult(exec.execute(&show));
    }

    // =========================
    // 6. 查询验证（全表扫描 students）
    // =========================
    {
        PlanNode select;
        select.type = PlanType::Project;
        select.table_name = "students";
        select.columns = {"*"}; // SELECT *
        auto result = exec.execute(&select);
        std::cout << "\n== Current students ==\n";
        printResult(result);
    }

    return 0;
}
