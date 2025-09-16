#pragma once

#include "../engine/operators/plan_node.h"
#include <memory>
#include <string>
#include <vector>

namespace minidb {

// 轻量规则优化入口：对计划树做局部重写（谓词下推、投影剪枝、Limit 下推占位）
std::unique_ptr<PlanNode> OptimizePlan(std::unique_ptr<PlanNode> plan);

} // namespace minidb


