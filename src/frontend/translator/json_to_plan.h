#pragma once

#include "../../util/json.hpp" // nlohmann/json 单头文件库
#include "../../sql_compiler/lexer/lexer.h"
#include "../../sql_compiler/parser/parser.h"
// #include "../../sql_compiler/semantic/semantic_analyzer.h"
#include <string>
#include <vector>
#include <memory>

using json = nlohmann::json;

// 前向声明
struct PlanNode;

/**
 * JsonToPlan: 把 SQL 的 JSON 表达转成执行计划 PlanNode
 */

class JsonToPlan
{
public:
    static std::unique_ptr<PlanNode> translate(const json &j);
};
