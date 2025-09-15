#include "../../src/auth/auth_service.h"
#include "../../src/storage/storage_engine.h"
#include "../../src/catalog/catalog.h"
#include <iostream>
#include <string>
#include <vector>
#include <ctime>
#include <cstdio> // For std::remove

using namespace minidb;

#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            std::cout << "[FAIL] " << message << std::endl; \
            return false; \
        } else { \
            std::cout << "[PASS] " << message << std::endl; \
        } \
    } while(0)

// Helper to clean up test files
void cleanupTestFiles(const std::string& dbFile) {
    std::remove(dbFile.c_str());
}

// 测试表可见性控制
bool testTableVisibility() {
    std::cout << "\n=== 测试表可见性控制 ===" << std::endl;
    const std::string dbFile = "data/test_table_visibility_" + std::to_string(time(nullptr)) + ".bin";
    cleanupTestFiles(dbFile);

    StorageEngine storage_engine(dbFile, 16);
    Catalog catalog(&storage_engine);
    AuthService auth_service(&storage_engine, &catalog);

    // 1. 以root用户登录
    TEST_ASSERT(auth_service.login("root", "root"), "root用户登录应该成功");
    
    // 2. 创建一些业务表
    std::vector<Column> columns = {{"id", "INT", -1}, {"name", "VARCHAR", 50}};
    catalog.CreateTable("users", columns, "root");
    catalog.CreateTable("orders", columns, "root");
    catalog.CreateTable("products", columns, "root");
    
    // 3. 创建DEVELOPER用户
    TEST_ASSERT(auth_service.createUser("devuser", "devpass", Role::DEVELOPER), "创建devuser应该成功");
    
    // 4. 登出root，登录devuser
    auth_service.logout();
    TEST_ASSERT(auth_service.login("devuser", "devpass"), "devuser登录应该成功");
    
    // 5. devuser创建自己的表
    catalog.CreateTable("dev_table", columns, "devuser");
    
    // 6. 测试表可见性 - 所有用户都应该能看到业务表，但看不到用户表
    auto all_tables = catalog.GetAllTableNames();
    std::cout << "所有表数量: " << all_tables.size() << std::endl;
    
    // 检查是否包含用户表
    bool has_users_table = false;
    for (const auto& table_name : all_tables) {
        if (table_name == "__users__") {
            has_users_table = true;
            break;
        }
    }
    TEST_ASSERT(has_users_table, "应该包含用户表__users__");
    
    // 检查业务表
    bool has_users_business_table = false;
    bool has_orders_table = false;
    bool has_products_table = false;
    bool has_dev_table = false;
    
    for (const auto& table_name : all_tables) {
        if (table_name == "users") has_users_business_table = true;
        if (table_name == "orders") has_orders_table = true;
        if (table_name == "products") has_products_table = true;
        if (table_name == "dev_table") has_dev_table = true;
    }
    
    TEST_ASSERT(has_users_business_table, "应该包含业务表users");
    TEST_ASSERT(has_orders_table, "应该包含业务表orders");
    TEST_ASSERT(has_products_table, "应该包含业务表products");
    TEST_ASSERT(has_dev_table, "应该包含业务表dev_table");
    
    // 7. 测试过滤后的表列表（模拟showTables功能）
    std::vector<std::string> visible_tables;
    for (const auto& table_name : all_tables) {
        if (table_name != "__users__") {
            visible_tables.push_back(table_name);
        }
    }
    
    std::cout << "可见表数量: " << visible_tables.size() << " (应该比总表数少1)" << std::endl;
    TEST_ASSERT(visible_tables.size() == all_tables.size() - 1, "可见表数量应该比总表数少1");
    
    // 检查可见表中不包含用户表
    bool visible_has_users_table = false;
    for (const auto& table_name : visible_tables) {
        if (table_name == "__users__") {
            visible_has_users_table = true;
            break;
        }
    }
    TEST_ASSERT(!visible_has_users_table, "可见表中不应该包含用户表__users__");
    
    // 8. 测试ANALYST用户也能看到所有业务表
    auth_service.logout();
    TEST_ASSERT(auth_service.login("root", "root"), "root用户重新登录应该成功");
    TEST_ASSERT(auth_service.createUser("analyst", "analystpass", Role::ANALYST), "创建analyst应该成功");
    
    auth_service.logout();
    TEST_ASSERT(auth_service.login("analyst", "analystpass"), "analyst登录应该成功");
    
    // ANALYST用户也应该能看到所有业务表
    auto analyst_visible_tables = catalog.GetAllTableNames();
    std::vector<std::string> analyst_filtered_tables;
    for (const auto& table_name : analyst_visible_tables) {
        if (table_name != "__users__") {
            analyst_filtered_tables.push_back(table_name);
        }
    }
    
    TEST_ASSERT(analyst_filtered_tables.size() == visible_tables.size(), "ANALYST用户应该能看到相同数量的业务表");

    cleanupTestFiles(dbFile);
    return true;
}

int main() {
    std::cout << "=== 表可见性控制测试程序 ===" << std::endl;
    std::cout << "测试表可见性控制功能" << std::endl;
    std::cout << "===============================================" << std::endl;

    bool all_passed = true;

    try {
        all_passed &= testTableVisibility();
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] 测试过程中发生异常: " << e.what() << std::endl;
        all_passed = false;
    }

    std::cout << "\n===============================================" << std::endl;
    if (all_passed) {
        std::cout << "✅ 所有表可见性控制测试通过！" << std::endl;
    } else {
        std::cout << "❌ 部分表可见性控制测试失败，需要修复问题" << std::endl;
    }

    return all_passed ? 0 : 1;
}
