/**
 * 接收 PlanNode，然后调度执行（目前只打印，之后再接存储）
 */

#include "executor.h"
#include <iostream>
#include <cstring>
#include "../../util/logger.h"
#include "../../storage/page/disk_manager.h"

static Logger logger("minidb.log");

namespace minidb
{

    void Executor::execute(PlanNode *node)
    {
        switch (node->type)
        {
        case PlanType::CreateTable:
            logger.log("CREATE TABLE " + node->table_name);
            std::cout << "[执行器] 创建表: " << node->table_name << std::endl;
            // TODO: 调用 Catalog 创建表逻辑
            break;

        case PlanType::Insert:
            logger.log("INSERT INTO " + node->table_name);
            std::cout << "[执行器] 插入到表: " << node->table_name << std::endl;

            // --- Insert 写页逻辑 ---
            if (!disk_manager_)
            {
                std::cerr << "[Executor] DiskManager 未初始化！" << std::endl;
                break;
            }

            {
                // 构建 Row
                Row row;
                for (size_t i = 0; i < node->columns.size(); ++i)
                {
                    ColumnValue col;
                    col.col_name = "col" + std::to_string(i); // 简单占位列名
                    col.value = node->columns[i];
                    row.columns.push_back(col);
                }

                // 分配页面
                page_id_t page_id = disk_manager_->AllocatePage();
                Page page(page_id);

                // 序列化 row 到 page.data_
                char *data = page.GetData();
                size_t offset = 0;
                for (auto &col : row.columns)
                {
                    // 简单固定长度序列化，每列 64 字节
                    std::string val = col.value;
                    if (val.size() > 64)
                        val = val.substr(0, 64);
                    std::memset(data + offset, 0, 64);
                    std::memcpy(data + offset, val.data(), val.size());
                    offset += 64;
                }

                // 写入磁盘
                auto status = disk_manager_->WritePage(page_id, data);
                if (status != Status::OK)
                {
                    std::cerr << "[Executor] 写页失败！" << std::endl;
                }
                else
                {
                    std::cout << "[Executor] 写页成功，page_id=" << page_id << std::endl;
                }
            }
            break;

        case PlanType::SeqScan:
            logger.log("SELECT * FROM " + node->table_name);
            std::cout << "[执行器] 查询表: " << node->table_name << std::endl;
            // TODO: 实现 SeqScan
            break;

        case PlanType::Delete:
            logger.log("DELETE FROM " + node->table_name);
            std::cout << "[执行器] 删除表: " << node->table_name << std::endl;
            // TODO: 实现 Delete
            break;

        case PlanType::Filter:
            logger.log("FILTER on " + node->predicate);
            std::cout << "[执行器] 过滤条件: " << node->predicate << std::endl;
            // TODO: 调用 Filter 函数
            break;

        case PlanType::Project:
            logger.log("PROJECT columns");
            std::cout << "[执行器] 投影列: ";
            for (auto &col : node->columns)
                std::cout << col << " ";
            std::cout << std::endl;
            // TODO: 调用 Project 函数
            break;

        default:
            std::cerr << "[执行器] 未知 PlanNode 类型" << std::endl;
        }
    }

} // namespace minidb
