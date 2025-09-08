/**
 * 定义页结构和行映射
 */
#pragma once
#include "Row.h"
#include <vector>

struct Slot
{
    int offset;
    int length;
    bool is_occupied;
};

struct Page
{
    int page_id;
    std::vector<Slot> slots;
    std::vector<Row> rows; // 内存模拟
};
