/**
 * 接收 PlanNode，然后调度执行（目前只打印，之后再接存储）
 */

#include "Executor.h"
#include <iostream>
#include "../../../tests/unit/Logger.h"

static Logger logger("minidb.log");

void Executor::execute(PlanNode *node)
{
    switch (node->type)
    {
    case PlanType::CreateTable:
        logger.log("CREATE TABLE " + node->table_name);
        std::cout << "[执行器] 创建表: " << node->table_name << std::endl;
        break;

    case PlanType::Insert:
        logger.log("INSERT INTO " + node->table_name);
        std::cout << "[执行器] 插入到表: " << node->table_name << std::endl;
        break;

    case PlanType::SeqScan:
        logger.log("SELECT * FROM " + node->table_name);
        std::cout << "[执行器] 查询表: " << node->table_name << std::endl;
        break;

    case PlanType::Delete:
        logger.log("DELETE FROM " + node->table_name);
        std::cout << "[执行器] 删除表: " << node->table_name << std::endl;
        break;

    case PlanType::Filter:
        logger.log("FILTER on " + node->predicate);
        std::cout << "[执行器] 过滤条件: " << node->predicate << std::endl;
        break;

    case PlanType::Project:
        logger.log("PROJECT columns");
        std::cout << "[执行器] 投影列: ";
        for (auto &col : node->columns)
        {
            std::cout << col << " ";
        }
        std::cout << std::endl;
        break;

    default:
        std::cerr << "[执行器] 未知 PlanNode 类型" << std::endl;
    }
}
