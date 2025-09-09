#include "../../src/frontend/translator/json_to_plan.h"
#include <iostream>
#include <cassert>

int main()
{
    // 构造一个 JSON
    nlohmann::json j = {
        {"type", "Project"},
        {"columns", {"name", "age"}},
        {"child", {{"type", "Filter"}, {"predicate", "age > 18"}, {"child", {{"type", "SeqScan"}, {"table_name", "students"}}}}}};

    // 调用 translate
    std::unique_ptr<PlanNode> plan = JsonToPlan::translate(j);

    // 测试根节点
    assert(plan->type == PlanType::Project);
    assert(plan->columns.size() == 2);
    assert(plan->columns[0] == "name");

    // 测试 child
    assert(plan->children.size() == 1);
    auto &filterNode = plan->children[0];
    assert(filterNode->type == PlanType::Filter);
    assert(filterNode->predicate == "age > 18");

    // 测试 child->child
    assert(filterNode->children.size() == 1);
    auto &scanNode = filterNode->children[0];
    assert(scanNode->type == PlanType::SeqScan);
    assert(scanNode->table_name == "students");

    std::cout << "JsonToPlan test passed!" << std::endl;
    return 0;
}
