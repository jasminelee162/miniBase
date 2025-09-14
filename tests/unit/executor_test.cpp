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

    /* 2. 插入多行数据 */
    {
        std::vector<std::vector<std::string>> vals = {
            {"201", "Alice Smith", "Math", "5"},
            {"202", "Bob Johnson", "English", "8"},
            {"203", "Charlie Lee", "Math", "3"},
            {"204", "Diana King", "English", "7"},
            {"205", "Eve Adams", "Math", "10"},
            {"206", "Frank Green", "Math", "12"},
            {"207", "Grace White", "English", "15"},
            {"208", "Hannah Brown", "Physics", "6"},
            {"209", "Ian Kim", "History", "10"},
            {"210", "Jack Davis", "Chemistry", "5"}};

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

    /* 3. 全表扫描 */
    std::cout << "\n== After Insert ==\n";
    {
        PlanNode scan;
        scan.type = PlanType::SeqScan;
        scan.table_name = "teachers";
        printResult(exec.execute(&scan));
    }

    /* 4. Filter 体验 > 5 */
    std::cout << "\n== Filter experience > 5 ==\n";
    {
        PlanNode f;
        f.type = PlanType::Filter;
        f.table_name = "teachers";
        f.predicate = "experience>5";
        printResult(exec.execute(&f));
    }

    /* 5. Project 两列 */
    std::cout << "\n== Project full_name, subject ==\n";
    {
        PlanNode p;
        p.type = PlanType::Project;
        p.table_name = "teachers";
        p.columns = {"full_name", "subject"};
        printResult(exec.execute(&p));
    }

    /* 6. Update Bob -> 9 年经验 */
    std::cout << "\n== Update Bob Johnson -> experience=9 ==\n";
    {
        PlanNode u;
        u.type = PlanType::Update;
        u.table_name = "teachers";
        u.predicate = "full_name=Bob Johnson";
        u.set_values = {{"experience", "9"}};
        exec.execute(&u);
    }

    std::cout << "\n== After Update ==\n";
    {
        PlanNode s;
        s.type = PlanType::SeqScan;
        s.table_name = "teachers";
        printResult(exec.execute(&s));
    }

    /* 7. Delete Alice */
    std::cout << "\n== Delete teacher_id=201 ==\n";
    {
        PlanNode d;
        d.type = PlanType::Delete;
        d.table_name = "teachers";
        d.predicate = "teacher_id=201";
        exec.execute(&d);
    }

    std::cout << "\n== After Delete ==\n";
    {
        PlanNode s;
        s.type = PlanType::SeqScan;
        s.table_name = "teachers";
        printResult(exec.execute(&s));
    }

    /* 8. Group By + Having */
    std::cout << "\n== Group By subject, SUM(experience) ==\n";
    {
        PlanNode g;
        g.type = PlanType::GroupBy;
        g.table_name = "teachers";
        g.group_keys = {"subject"};
        g.aggregates = {{"SUM", "experience", "total_exp"}};
        printResult(exec.execute(&g));
    }

    std::cout << "\n== Having total_exp>5 ==\n";
    {
        PlanNode g;
        g.type = PlanType::GroupBy;
        g.table_name = "teachers";
        g.group_keys = {"subject"};
        g.aggregates = {{"SUM", "experience", "total_exp"}};
        g.having_predicate = "total_exp>5";
        printResult(exec.execute(&g));
    }

    /* 9. 建 departments 并插 2 行 */
    {
        PlanNode ct;
        ct.type = PlanType::CreateTable;
        ct.table_name = "departments";
        ct.table_columns = {{"dept_id", "INT", -1}, {"dept_name", "VARCHAR", 50}};
        exec.execute(&ct);
    }

    {
        std::vector<std::vector<std::string>> d = {
            {"1", "Math"},
            {"2", "English"},
            {"3", "Physics"},
            {"4", "History"},
            {"5", "Chemistry"}};

        for (auto &v : d)
        {
            PlanNode ins;
            ins.type = PlanType::Insert;
            ins.table_name = "departments";
            ins.columns = {"dept_id", "dept_name"};
            ins.values = {v};
            exec.execute(&ins);
        }
    }

    /* 10. 两表连接 */
    std::cout << "\n== Join teachers.subject = departments.dept_name ==\n";
    {
        auto j = std::make_unique<PlanNode>();
        j->type = PlanType::Join;
        j->from_tables = {"teachers", "departments"};
        j->columns = {"*"};
        j->predicate = "teachers.subject=departments.dept_name";
        printResult(exec.execute(j.get()));
    }

    /* 11. OrderBy 测试 */
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