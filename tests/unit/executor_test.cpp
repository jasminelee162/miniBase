#include "../../src/engine/executor/executor.h"
#include "../../src/engine/operators/plan_node.h"
#include "../../src/catalog/catalog.h"
#include <iostream>
#include <memory>
#include <windows.h>

// 打印结果工具函数
void printResult(const std::vector<Row> &rows)
{
    if (rows.empty())
    {
        std::cout << "(no rows)" << std::endl;
        return;
    }
    for (const auto &row : rows)
    {
        std::cout << row.toString() << std::endl;
    }
}

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
        exec.execute(&create); // 返回值可忽略
    }

    // 5. INSERT 新数据
    {
        std::vector<std::vector<std::string>> rows = {
            {"201", "Alice Smith", "Math", "5"},
            {"202", "Bob Johnson", "English", "8"},
            {"203", "Carol Lee", "Physics", "3"},
            {"204", "David Kim", "History", "10"},
            {"205", "Eva Brown", "Chemistry", "6"},
            {"206", "Frank Green", "Math", "12"},
            {"207", "Grace White", "English", "15"}};

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
        auto result = exec.execute(&scan);
        printResult(result);
    }

    // 6. FILTER experience > 5
    std::cout << "\n== Filter experience > 5 ==" << std::endl;
    {
        PlanNode filter;
        filter.type = PlanType::Filter;
        filter.table_name = "teachers";
        filter.predicate = "experience>5";
        auto result = exec.execute(&filter);
        printResult(result);
    }

    // 7. PROJECT full_name, subject
    std::cout << "\n== Project full_name, subject ==" << std::endl;
    {
        PlanNode project;
        project.type = PlanType::Project;
        project.table_name = "teachers";
        project.columns = {"full_name", "subject"};
        auto result = exec.execute(&project);
        printResult(result);
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
        auto result = exec.execute(&scan);
        printResult(result);
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

    std::cout << "\n== After Delete ==" << std::endl;
    {
        PlanNode scan;
        scan.type = PlanType::SeqScan;
        scan.table_name = "teachers";
        auto result = exec.execute(&scan);
        printResult(result);
    }

    // 10. GROUP BY subject, SUM(experience)
    std::cout << "\n== Group By subject, SUM(experience) ==" << std::endl;
    {
        PlanNode group;
        group.type = PlanType::GroupBy;
        group.table_name = "teachers";
        group.group_keys = {"subject"};
        group.aggregates = {
            {"SUM", "experience", "total_exp"}};
        auto result = exec.execute(&group);
        printResult(result);
    }

    // 11. GROUP BY subject, SUM(experience) HAVING total_exp > 15
    std::cout << "\n== Group By subject, SUM(experience) HAVING total_exp > 15 ==" << std::endl;
    {
        PlanNode group;
        group.type = PlanType::GroupBy;
        group.table_name = "teachers";
        group.group_keys = {"subject"};
        group.aggregates = {
            {"SUM", "experience", "total_exp"}};
        group.having_predicate = "total_exp>15";
        auto result = exec.execute(&group);
        printResult(result);
    }

    return 0;
}
