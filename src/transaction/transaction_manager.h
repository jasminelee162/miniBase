#pragma once
#include <unordered_map>
#include <iostream>
#include <vector>
#include <memory> // 包含 std::shared_ptr 的头文件
#include "../storage/page/page.h"
#include "../storage/storage_engine.h"

namespace minidb
{

    class TransactionManager
    {
    public:
        // 修改构造函数以接受 std::shared_ptr<StorageEngine>
        TransactionManager(std::shared_ptr<StorageEngine> engine) : engine_(engine) {}

        void Begin()
        {
            active_ = true;
            modified_pages_.clear();
            std::cout << "[Transaction] START TRANSACTION" << std::endl;
        }

        void Commit()
        {
            if (!active_)
                return;
            // 标记事务结束，修改过的页全部刷盘
            for (auto pid : modified_pages_)
            {
                engine_->PutPage(pid, true);
            }
            modified_pages_.clear();
            active_ = false;
            std::cout << "[Transaction] COMMIT" << std::endl;
        }

        void Rollback()
        {
            if (!active_)
                return;
            // 撤销：把修改过的页丢弃（不标脏，不写回）
            for (auto pid : modified_pages_)
            {
                engine_->PutPage(pid, false);
            }
            modified_pages_.clear();
            active_ = false;
            std::cout << "[Transaction] ROLLBACK" << std::endl;
        }

        void RecordPageModification(page_id_t pid)
        {
            if (active_)
            {
                modified_pages_.push_back(pid);
            }
        }

        bool IsActive() const { return active_; }

    private:
        std::shared_ptr<StorageEngine> engine_; // 修改为 shared_ptr 类型
        bool active_ = false;
        std::vector<page_id_t> modified_pages_;
    };

} // namespace minidb