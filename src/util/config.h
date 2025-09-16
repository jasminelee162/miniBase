// src/util/config.h
#pragma once
#include <cstdint>
#include <cstddef>

namespace minidb
{

    // 基础配置
    constexpr size_t PAGE_SIZE = 4096;
    constexpr size_t BUFFER_POOL_SIZE = 128;   //缓冲池页数（默认，可被动态覆盖）
    constexpr size_t MAX_PAGES = 1000000;
    // 默认虚拟磁盘大小（用于新建时预分配），可按需调整
    // constexpr size_t DEFAULT_DISK_SIZE_BYTES = 10 * 1024 * 1024; // 10MB
    constexpr size_t DEFAULT_DISK_SIZE_BYTES = 160 * 1024;   //128KB
    constexpr size_t DEFAULT_MAX_PAGES = DEFAULT_DISK_SIZE_BYTES / PAGE_SIZE;

    // 可选：是否输出存储层日志
    constexpr bool ENABLE_STORAGE_LOG = true;

    // 运行时可调参数（通过环境变量或配置加载时覆盖）
    struct RuntimeConfig {
        size_t buffer_pool_pages = BUFFER_POOL_SIZE;
        size_t io_worker_threads = 1;
        size_t io_batch_max = 64;
        uint32_t bpm_flush_interval_ms = 200;
        size_t bpm_max_flush_per_cycle = 64;
        bool bpm_autoresize = true;
        bool bpm_readahead = true;
        uint32_t bpm_readahead_window = 4;
    };

    // 提供获取全局可写配置实例的接口
    RuntimeConfig& GetRuntimeConfig();

    // 替换策略
    enum class ReplacementPolicy
    {
        LRU = 0,
        FIFO = 1
    };

    // 全局默认替换策略（编译期常量）
    constexpr ReplacementPolicy DEFAULT_REPLACEMENT_POLICY = ReplacementPolicy::LRU;

    // 类型定义
    using page_id_t = uint32_t;
    using frame_id_t = size_t;
    using lsn_t = uint64_t;

    // 特殊值：页号从0开始，因此无效页应取最大值，避免与合法页0冲突
    constexpr page_id_t INVALID_PAGE_ID = UINT32_MAX;
    constexpr frame_id_t INVALID_FRAME_ID = SIZE_MAX;

}