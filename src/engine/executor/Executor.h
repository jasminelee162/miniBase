#pragma once
#include "../operators/planNode.h"

class Executor
{
public:
    void execute(PlanNode *node);
};
