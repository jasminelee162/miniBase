#pragma once
#include "PlanNode.h"
#include "Page.h"
#include <vector>
#include <string>

class Executor
{
public:
    std::vector<Row> SeqScan(Page &page);
    std::vector<Row> Filter(const std::vector<Row> &rows, const std::string &predicate);
    std::vector<Row> Project(const std::vector<Row> &rows, const std::vector<std::string> &cols);
};
