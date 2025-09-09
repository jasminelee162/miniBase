#pragma once

#include "../../engine/operators/plan_node.h"
#include "../../util/json.hpp" // nlohmann/json 单头文件库
#include <string>
#include <vector>

using json = nlohmann::json;

/**
 * JsonToPlan: 把 SQL 的 JSON 表达转成执行计划 PlanNode
 */

using json = nlohmann::json;

class JsonToPlan
{
public:
    static std::unique_ptr<PlanNode> translate(const json &j);
};
