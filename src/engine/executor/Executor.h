#pragma once
#include "../operators/PlanNode.h"

class Executor
{
public:
    void execute(PlanNode *node);
};
