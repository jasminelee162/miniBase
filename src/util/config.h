// src/util/config.h
#pragma once
#include <cstdint>
#include <cstddef>

namespace minidb
{

    // 基础配置
    constexpr size_t PAGE_SIZE = 4096;
    constexpr size_t BUFFER_POOL_SIZE = 128;
    constexpr size_t MAX_PAGES = 1000000;

    // 可选：是否输出存储层日志
    constexpr bool ENABLE_STORAGE_LOG = false;

    // 替换策略
    enum class ReplacementPolicy
    {
        LRU = 0,
        FIFO = 1
    };

    // 类型定义
    using page_id_t = uint32_t;
    using frame_id_t = size_t;
    using lsn_t = uint64_t;

    // 特殊值
    constexpr page_id_t INVALID_PAGE_ID = 0;
    constexpr frame_id_t INVALID_FRAME_ID = SIZE_MAX;

}