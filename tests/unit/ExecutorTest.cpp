#include "../../src/engine/operators/Executor.h"
#include "../../src/engine/operators/Page.h"
#include <iostream>

int main()
{
    Page page;
    page.page_id = 1;
    Row row1 = {{{"id", "1"}, {"name", "Alice"}, {"age", "20"}}};
    Row row2 = {{{"id", "2"}, {"name", "Bob"}, {"age", "25"}}};
    page.rows.push_back(row1);
    page.rows.push_back(row2);

    Executor exe;
    auto scanned = exe.SeqScan(page);
    auto filtered = exe.Filter(scanned, "Alice");
    auto projected = exe.Project(filtered, {"id", "name"});

    for (auto &row : projected)
    {
        for (auto &col : row.columns)
            std::cout << col.col_name << ":" << col.value << " ";
        std::cout << std::endl;
    }
    return 0;
}
