/**
 * 把 SQL 字符串转成 PlanNode
 */

#pragma once
#include "../../engine/operators/plan_node.h"
#include <string>

class SQLParser
{
public:
    static PlanNode *parse(const std::string &sql);
};
