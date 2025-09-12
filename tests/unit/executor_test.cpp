#include "../../src/engine/executor/executor.h"
#include "../../src/engine/operators/plan_node.h"
#include <iostream>
#include <memory>
#include <windows.h>

int main()
{
    SetConsoleOutputCP(CP_UTF8);
    // 1. Create StorageEngine
    auto engine = std::make_shared<minidb::StorageEngine>("minidb.db", 10);

    // 2. Create Executor
    minidb::Executor exec(engine);

    auto catalog = std::make_shared<minidb::Catalog>("catalog.dat");
    exec.SetCatalog(catalog);

    // 3. CREATE TABLE students
    {
        PlanNode create;
        create.type = PlanType::CreateTable;
        create.table_name = "students";
        create.table_columns = {
            {"id", "INT", -1},
            {"name", "VARCHAR", 50},
            {"age", "INT", -1}}; // ✅ 用 table_columns，而不是 columns
        exec.execute(&create);
    }

    // 4. INSERT rows
    {
        PlanNode insert;
        insert.type = PlanType::Insert;
        insert.table_name = "students";
        insert.columns = {"id", "name", "age"}; // ✅ 插入的列

        insert.values = {{"1", "Alice", "20"}}; // ✅ 插入的数据
        exec.execute(&insert);

        insert.values = {{"2", "Bob", "25"}};
        exec.execute(&insert);

        insert.values = {{"3", "Charlie", "17"}};
        exec.execute(&insert);
    }

    std::cout << "\n== After Insert ==" << std::endl;
    {
        PlanNode scan;
        scan.type = PlanType::SeqScan;
        scan.table_name = "students";
        exec.execute(&scan);
    }

    // 5. FILTER (age > 18)
    std::cout << "\n== Filter age > 18 ==" << std::endl;
    {
        PlanNode filter;
        filter.type = PlanType::Filter;
        filter.table_name = "students";
        filter.predicate = "age>18";
        exec.execute(&filter);
    }

    // 6. PROJECT (name, age)
    std::cout << "\n== Project name, age ==" << std::endl;
    {
        PlanNode project;
        project.type = PlanType::Project;
        project.table_name = "students";
        project.columns = {"name", "age"};
        exec.execute(&project);
    }

    // 7. UPDATE (set age=30 where name=Alice)
    std::cout << "\n== Update Alice -> age=30 ==" << std::endl;
    {
        PlanNode update;
        update.type = PlanType::Update;
        update.table_name = "students";
        update.predicate = "name=Alice";
        update.set_values = {{"age", "30"}};
        exec.execute(&update);
    }

    std::cout << "\n== After Update ==" << std::endl;
    {
        PlanNode scan;
        scan.type = PlanType::SeqScan;
        scan.table_name = "students";
        exec.execute(&scan);
    }

    // 8. DELETE (id=2)
    std::cout << "\n== Delete id=2 ==" << std::endl;
    {
        PlanNode del;
        del.type = PlanType::Delete;
        del.table_name = "students";
        del.predicate = "id=2";
        exec.execute(&del);
    }

    std::cout << "\n== Final Table ==" << std::endl;
    {
        PlanNode scan;
        scan.type = PlanType::SeqScan;
        scan.table_name = "students";
        exec.execute(&scan);
    }

    return 0;
}
