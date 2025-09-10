#include "storage/storage_engine.h"
#include "util/logger.h" //////
#include "storage/storage_logger.h"
#include "util/config.h"
#include <iostream>
#include <sstream>

namespace minidb
{

#ifdef PROJECT_ROOT_DIR
    static Logger g_storage_logger_engine(std::string(PROJECT_ROOT_DIR) + "/logs/storage.log");
#else
    static Logger g_storage_logger_engine("storage.log");
#endif

    StorageEngine::StorageEngine(const std::string &db_file, size_t buffer_pool_size)
        : disk_manager_(std::make_unique<DiskManager>(db_file)),
          buffer_pool_manager_(std::make_unique<BufferPoolManager>(buffer_pool_size, disk_manager_.get())),
          db_file_(db_file)
    {
        // 初始化storage logger
        initStorageLogger("storage.log", LogLevel::INFO);
    }

    StorageEngine::~StorageEngine()
    {
        Shutdown();
    }
    // 获取一页
    Page *StorageEngine::GetPage(page_id_t page_id)
    {
        return buffer_pool_manager_->FetchPage(page_id);
    }
    // 申请新页
    Page *StorageEngine::CreatePage(page_id_t *page_id)
    {
        return buffer_pool_manager_->NewPage(page_id);
    }
    // 用完页后归还缓存，标记脏否
    bool StorageEngine::PutPage(page_id_t page_id, bool is_dirty)
    {
        return buffer_pool_manager_->UnpinPage(page_id, is_dirty);
    }
    // 删除一页
    bool StorageEngine::RemovePage(page_id_t page_id)
    {
        return buffer_pool_manager_->DeletePage(page_id);
    }
    // 获取多页
    std::vector<Page *> StorageEngine::GetPages(const std::vector<page_id_t> &page_ids)
    {
        std::vector<Page *> result;
        result.reserve(page_ids.size());
        for (auto pid : page_ids)
        {
            result.push_back(buffer_pool_manager_->FetchPage(pid));
        }
        return result;
    }
    // 刷脏页并关闭文件
    void StorageEngine::Shutdown()
    {
        if (is_shutdown_.exchange(true))
            return;
        if (buffer_pool_manager_)
            buffer_pool_manager_->FlushAllPages();
        if (disk_manager_)
            disk_manager_->Shutdown();
    }
    // 只刷脏页不关闭文件
    void StorageEngine::Checkpoint()
    {
        if (buffer_pool_manager_)
            buffer_pool_manager_->FlushAllPages();
    }
    // 打印统计信息
    void StorageEngine::PrintStats() const
    {
        std::ostringstream oss;
        oss << "BufferPoolSize=" << GetBufferPoolSize()
            << ", HitRate=" << GetCacheHitRate()
            << ", Replacements=" << GetNumReplacements()
            << ", Writebacks=" << GetNumWritebacks();
        const std::string msg = oss.str();
        std::cout << msg << std::endl;

        if (g_storage_logger)
        {
            g_storage_logger->info("ENGINE", msg);
            g_storage_logger->printStatistics();
        }
    }
    // 缓存命中率
    double StorageEngine::GetCacheHitRate() const
    {
        return buffer_pool_manager_ ? buffer_pool_manager_->GetHitRate() : 0.0;
    }
    // 缓存池大小
    size_t StorageEngine::GetBufferPoolSize() const
    {
        return buffer_pool_manager_ ? buffer_pool_manager_->GetPoolSize() : 0;
    }
    // 调整缓存池大小
    bool StorageEngine::AdjustBufferPoolSize(size_t new_size)
    {
        return buffer_pool_manager_ ? buffer_pool_manager_->ResizePool(new_size) : false;
    }
    // 替换次数
    size_t StorageEngine::GetNumReplacements() const
    {
        return buffer_pool_manager_ ? buffer_pool_manager_->GetNumReplacements() : 0;
    }
    // 写回次数
    size_t StorageEngine::GetNumWritebacks() const
    {
        return buffer_pool_manager_ ? buffer_pool_manager_->GetNumWritebacks() : 0;
    }
    // 设置替换策略
    void StorageEngine::SetReplacementPolicy(ReplacementPolicy policy)
    {
        if (buffer_pool_manager_)
            buffer_pool_manager_->SetPolicy(policy);
    }

    // 页数量
    size_t StorageEngine::GetNumPages() const
    {
        if (!disk_manager_)
            return 0;
        return disk_manager_->GetNumPages();
    }

    // 创建表
    bool StorageEngine::CreateTable(const std::string &table_name, const std::vector<std::string> &columns)
    {
        std::lock_guard<std::mutex> lock(schema_mutex_);

        // 检查是否已有该表
        auto it = std::find_if(table_schemas_.begin(), table_schemas_.end(),
                               [&](const TableSchema &s)
                               { return s.table_name == table_name; });
        if (it != table_schemas_.end())
            return false; // 已存在

        TableSchema schema;
        schema.table_name = table_name;
        schema.columns = columns;
        table_schemas_.push_back(schema);
        return true;
    }

    // 获取表列名
    std::vector<std::string> StorageEngine::GetTableColumns(const std::string &table_name)
    {
        std::lock_guard<std::mutex> lock(schema_mutex_);
        for (auto &schema : table_schemas_)
        {
            if (schema.table_name == table_name)
                return schema.columns;
        }
        return {};
    }

} // namespace minidb
