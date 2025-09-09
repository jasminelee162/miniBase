#include "../../src/engine/executor/executor.h"
#include <iostream>

int main()
{
    minidb::DiskManager dm("minidb.db"); // 1. 先创建 DiskManager
    minidb::Executor exec(&dm);          // 2. 再创建 Executor

    // 1. CREATE TABLE
    PlanNode create;
    create.type = PlanType::CreateTable;
    create.table_name = "students";
    exec.execute(&create);

    // 2. INSERT
    PlanNode insert;
    insert.type = PlanType::Insert;
    insert.table_name = "students";
    insert.columns = {"id=1", "name='Alice'"};
    exec.execute(&insert);

    // 3. DELETE
    PlanNode del;
    del.type = PlanType::Delete;
    del.table_name = "students";
    del.predicate = "id=1";
    exec.execute(&del);

    // 4. SEQ SCAN
    PlanNode scan;
    scan.type = PlanType::SeqScan;
    scan.table_name = "students";
    exec.execute(&scan);

    // 5. FILTER
    PlanNode filter;
    filter.type = PlanType::Filter;
    filter.predicate = "age > 18";
    exec.execute(&filter);

    // 6. PROJECT
    PlanNode project;
    project.type = PlanType::Project;
    project.columns = {"name", "age"};
    exec.execute(&project);

    return 0;
}
