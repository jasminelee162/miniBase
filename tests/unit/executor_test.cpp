#include "../../src/engine/executor/executor.h"
#include "../../src/engine/operators/plan_node.h"
#include "../../src/catalog/catalog.h"
#include <iostream>
#include <memory>
#include <windows.h>

int main()
{
    SetConsoleOutputCP(CP_UTF8);

    // 1. 创建 StorageEngine (10 页)
    auto engine = std::make_shared<minidb::StorageEngine>("school.db", 10);

    // 2. 创建 Executor
    minidb::Executor exec(engine);

    // 3. 创建 Catalog 并关联 Executor
    auto catalog = std::make_shared<minidb::Catalog>(engine.get());
    exec.SetCatalog(catalog);

    // 4. CREATE TABLE teachers
    {
        PlanNode create;
        create.type = PlanType::CreateTable;
        create.table_name = "teachers";
        create.table_columns = {
            minidb::Column{"teacher_id", "INT", -1},
            minidb::Column{"full_name", "VARCHAR", 100},
            minidb::Column{"subject", "VARCHAR", 50},
            minidb::Column{"experience", "INT", -1}};
        exec.execute(&create);
    }

    // 5. INSERT 新数据
    {
        std::vector<std::vector<std::string>> rows = {
            {"201", "Alice Smith", "Math", "5"},
            {"202", "Bob Johnson", "English", "8"},
            {"203", "Carol Lee", "Physics", "3"},
            {"204", "David Kim", "History", "10"},
            {"205", "Eva Brown", "Chemistry", "6"}};

        for (auto &vals : rows)
        {
            PlanNode insert;
            insert.type = PlanType::Insert;
            insert.table_name = "teachers";
            insert.columns = {"teacher_id", "full_name", "subject", "experience"};
            insert.values = {vals};
            exec.execute(&insert);
        }
    }

    std::cout << "\n== After Insert ==" << std::endl;
    {
        PlanNode scan;
        scan.type = PlanType::SeqScan;
        scan.table_name = "teachers";
        exec.execute(&scan);
    }

    // 6. FILTER experience > 5
    std::cout << "\n== Filter experience > 5 ==" << std::endl;
    {
        PlanNode filter;
        filter.type = PlanType::Filter;
        filter.table_name = "teachers";
        filter.predicate = "experience>5";
        exec.execute(&filter);
    }

    // 7. PROJECT full_name, subject
    std::cout << "\n== Project full_name, subject ==" << std::endl;
    {
        PlanNode project;
        project.type = PlanType::Project;
        project.table_name = "teachers";
        project.columns = {"full_name", "subject"};
        exec.execute(&project);
    }

    // 8. UPDATE Bob Johnson -> experience=9
    std::cout << "\n== Update Bob Johnson -> experience=9 ==" << std::endl;
    {
        PlanNode update;
        update.type = PlanType::Update;
        update.table_name = "teachers";
        update.predicate = "full_name=Bob Johnson";
        update.set_values = {{"experience", "9"}};
        exec.execute(&update);
    }

    std::cout << "\n== After Update ==" << std::endl;
    {
        PlanNode scan;
        scan.type = PlanType::SeqScan;
        scan.table_name = "teachers";
        exec.execute(&scan);
    }

    // 9. DELETE teacher_id=203 (Carol Lee)
    std::cout << "\n== Delete teacher_id=203 ==" << std::endl;
    {
        PlanNode del;
        del.type = PlanType::Delete;
        del.table_name = "teachers";
        del.predicate = "teacher_id=203";
        exec.execute(&del);
    }

    std::cout << "\n== Final Table ==" << std::endl;
    {
        PlanNode scan;
        scan.type = PlanType::SeqScan;
        scan.table_name = "teachers";
        exec.execute(&scan);
    }

    return 0;
}
