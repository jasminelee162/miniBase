#include "../../src/engine/executor/executor.h"
#include "../../src/engine/operators/plan_node.h"
#include "../../src/catalog/catalog.h"
#include "../../src/auth/auth_service.h"
#include "../../src/auth/permission_checker.h"
#include "../../src/cli/AuthCLI.h"

#include <iostream>
#include <memory>
#ifdef _WIN32
#include <windows.h>
#endif

void printResult(const std::vector<Row> &rows)
{
    if (rows.empty())
    {
        std::cout << "(no rows)\n";
        return;
    }
    for (const auto &r : rows)
        std::cout << r.toString() << '\n';
}

int main()
{
    // 初始化存储引擎和 catalog
    auto se = std::make_shared<minidb::StorageEngine>("data/mini.db", 16);
    auto catalog = std::make_shared<minidb::Catalog>(se.get());

    // 初始化权限和认证
    auto authService = std::make_unique<minidb::AuthService>(se.get(), catalog.get());
    auto permissionChecker = std::make_unique<minidb::PermissionChecker>(authService.get());

    // 初始化 Executor
    auto exec = std::make_unique<minidb::Executor>(catalog, permissionChecker.get());
    exec->SetAuthService(authService.get());
    exec->SetStorageEngine(se);

    // 模拟登录 root 用户
    if (!authService->login("root", "root"))
    {
        std::cerr << "登录失败！" << std::endl;
        return 1;
    }
    std::cout << "[Test] 登录成功，角色: "
              << authService->getCurrentUserRoleString()
              << std::endl;

    // -------- 构造 CreateIndex PlanNode --------
    auto node = std::make_unique<PlanNode>();
    node->type = PlanType::CreateIndex;
    node->table_name = "idx_test";
    node->index_name = "idx_id";
    node->index_cols = {"id"};
    node->index_type = "BPLUS";

    // 执行 CreateIndex
    try
    {
        exec->execute(node.get());
        std::cout << "[Test] 索引创建完成" << std::endl;
    }
    catch (const std::exception &ex)
    {
        std::cerr << "[Test] 创建索引失败: " << ex.what() << std::endl;
    }

    // -------- 可选：再执行一个 SELECT 来验证 --------
    auto selectNode = std::make_unique<PlanNode>();
    selectNode->type = PlanType::SeqScan;
    selectNode->table_name = "idx_test";
    selectNode->predicate = "id = 1"; // 简单条件

    try
    {
        auto rows = exec->execute(node.get());
        std::cout << "[Test] 查询完成，返回行数: " << rows.size() << std::endl;
    }
    catch (const std::exception &ex)
    {
        std::cerr << "[Test] 查询失败: " << ex.what() << std::endl;
    }

    // 登出
    authService->logout();
    std::cout << "[Test] 测试结束" << std::endl;
    return 0;
}