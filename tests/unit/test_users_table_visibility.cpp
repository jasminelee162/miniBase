#include "../simple_test_framework.h"
#include "../../src/storage/storage_engine.h"
#include "../../src/catalog/catalog.h"
#include "../../src/auth/auth_service.h"
#include <memory>
#include <iostream>

using namespace SimpleTest;
using namespace minidb;

static std::string test_db_file = "data/test_users_visibility.db";

static std::shared_ptr<StorageEngine> makeEngine()
{
    // 小缓冲池即可
    return std::make_shared<StorageEngine>(test_db_file, 8);
}

int main()
{
    TestSuite suite;

    suite.addTest("DBA can see and query __users__", [](){
        auto se = makeEngine();
        Catalog catalog(se.get());
        AuthService auth(se.get(), &catalog);
        ASSERT_TRUE(auth.login("root", "root"));

        // SHOW TABLES 可见（通过 Catalog::HasTable + 权限）
        ASSERT_TRUE(auth.checkTablePermission("__users__", Permission::SELECT));

        // 直接检查：读取用户表首页是否可行（权限由上层控制，这里断言权限为真）
        EXPECT_TRUE(catalog.HasTable("__users__"));
    });

    suite.addTest("DEVELOPER cannot see or query __users__", [](){
        auto se = makeEngine();
        Catalog catalog(se.get());
        AuthService auth(se.get(), &catalog);
        ASSERT_TRUE(auth.login("root", "root"));
        // 准备一个开发者账号
        EXPECT_TRUE(auth.createUser("dev_u", "dev_p", Role::DEVELOPER));

        auth.logout();
        ASSERT_TRUE(auth.login("dev_u", "dev_p"));

        // 对 __users__ 的 SELECT 应被拒绝
        ASSERT_FALSE(auth.checkTablePermission("__users__", Permission::SELECT));
        // 对 __users__ 的任何修改也应被拒绝
        ASSERT_FALSE(auth.checkTablePermission("__users__", Permission::INSERT));
        ASSERT_FALSE(auth.checkTablePermission("__users__", Permission::UPDATE));
        ASSERT_FALSE(auth.checkTablePermission("__users__", Permission::DELETE));
        ASSERT_FALSE(auth.checkTablePermission("__users__", Permission::DROP_TABLE));
    });

    suite.addTest("ANALYST cannot see or query __users__", [](){
        auto se = makeEngine();
        Catalog catalog(se.get());
        AuthService auth(se.get(), &catalog);
        ASSERT_TRUE(auth.login("root", "root"));
        // 准备一个分析师账号
        EXPECT_TRUE(auth.createUser("ana_u", "ana_p", Role::ANALYST));

        auth.logout();
        ASSERT_TRUE(auth.login("ana_u", "ana_p"));

        // 对 __users__ 的 SELECT/修改均应为 false（系统表仅 DBA 可见）
        ASSERT_FALSE(auth.checkTablePermission("__users__", Permission::SELECT));
        ASSERT_FALSE(auth.checkTablePermission("__users__", Permission::INSERT));
        ASSERT_FALSE(auth.checkTablePermission("__users__", Permission::UPDATE));
        ASSERT_FALSE(auth.checkTablePermission("__users__", Permission::DELETE));
        ASSERT_FALSE(auth.checkTablePermission("__users__", Permission::DROP_TABLE));
    });

    suite.runAll();
    return 0;
}
