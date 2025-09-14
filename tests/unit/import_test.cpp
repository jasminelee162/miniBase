#include "../../src/engine/executor/executor.h"
#include "../../src/catalog/catalog.h"
#include "../../src/tools/sql_import/sql_importer.h"
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
    SetConsoleOutputCP(CP_UTF8); // 解决 Windows 控制台中文乱码
#endif

    auto engine = std::make_shared<minidb::StorageEngine>("school3.db", 10);
    auto catalog = std::make_shared<minidb::Catalog>(engine.get());
    minidb::Executor exec(engine);
    exec.SetCatalog(catalog);

    minidb::SQLImporter importer(&exec, catalog.get());
    importer.ExecuteSQLFile("dump_all.sql");

    std::cout << "SQL 导入完成 ✅\n"
              << std::endl;

    // 🔍 导入后查询 student 表
    {
        PlanNode scan;
        scan.type = PlanType::SeqScan;
        scan.table_name = "students";

        auto rows = exec.execute(&scan);
        std::cout << "== 查询 student 表内容 ==" << std::endl;
        printResult(rows);
    }

    return 0;
}
