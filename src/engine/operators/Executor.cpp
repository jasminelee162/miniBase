/**
 * 模拟执行算子逻辑
 */
#include "Executor.h"

std::vector<Row> Executor::SeqScan(Page &page)
{
    return page.rows;
}

std::vector<Row> Executor::Filter(const std::vector<Row> &rows, const std::string &predicate)
{
    std::vector<Row> result;
    for (auto &row : rows)
    {
        for (auto &col : row.columns)
        {
            if (col.value.find(predicate) != std::string::npos)
            {
                result.push_back(row);
                break;
            }
        }
    }
    return result;
}

std::vector<Row> Executor::Project(const std::vector<Row> &rows, const std::vector<std::string> &cols)
{
    std::vector<Row> result;
    for (auto &row : rows)
    {
        Row r;
        for (auto &col_name : cols)
        {
            for (auto &col : row.columns)
            {
                if (col.col_name == col_name)
                    r.columns.push_back(col);
            }
        }
        result.push_back(r);
    }
    return result;
}
