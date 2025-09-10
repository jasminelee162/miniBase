// tests/unit/test_executor.cpp
#include "../../src/engine/executor/executor.h"
#include "../../src/frontend/translator/json_to_plan.h"
#include "../src/util/logger.h"
#include <iostream>

using namespace minidb;

void printSeparator()
{
    std::cout << "=============================" << std::endl;
}

int main()
{
    std::cout << "Running Executor Full Test..." << std::endl;

    Executor executor;
    executor.SetDiskManager(std::make_shared<DiskManager>("data/mini.db"));
    // executor.SetDiskManager(std::make_shared<DiskManager>("data/")); // 确保磁盘管理器路径存在

    JsonToPlan translator;

    // 1️⃣ CreateTable
    {
        std::string json_str = R"({
            "type": "CreateTable",
            "table_name": "users",
            "columns": ["id","name","age"]
        })";
        auto j = nlohmann::json::parse(json_str);
        auto plan = translator.translate(j);
        executor.execute(plan.get());
        printSeparator();
    }

    // 2️⃣ Insert
    {
        std::string json_str = R"({
            "type": "Insert",
            "table_name": "users",
            "columns": ["1","Alice","20"]
        })";
        auto j = nlohmann::json::parse(json_str);
        auto plan = translator.translate(j);
        executor.execute(plan.get());
        printSeparator();
    }

    // 3️⃣ Select (Filter + Project)
    {
        std::string json_str = R"({
            "type": "Select",
            "table_name": "users",
            "columns": ["name","age"],
            "predicate": "age > 18"
        })";
        auto j = nlohmann::json::parse(json_str);
        auto plan = translator.translate(j);
        std::cout << "[Test] Select(Filter + Project) before Update:" << std::endl;
        executor.execute(plan.get());
        printSeparator();
    }

    // 4️⃣ Update
    {
        std::string json_str = R"({
            "type": "Update",
            "table_name": "users",
            "set_columns": ["age"],
            "set_values": ["21"],
            "predicate": "name = Alice"
        })";
        auto j = nlohmann::json::parse(json_str);
        auto plan = translator.translate(j);
        executor.execute(plan.get());
        printSeparator();
    }

    // 5️⃣ Select after Update
    {
        std::string json_str = R"({
            "type": "Select",
            "table_name": "users",
            "columns": ["name","age"],
            "predicate": "age > 18"
        })";
        auto j = nlohmann::json::parse(json_str);
        auto plan = translator.translate(j);
        std::cout << "[Test] Select(Filter + Project) after Update:" << std::endl;
        executor.execute(plan.get());
        printSeparator();
    }

    // 6️⃣ Delete
    {
        std::string json_str = R"({
            "type": "Delete",
            "table_name": "users",
            "predicate": "name = Alice"
        })";
        auto j = nlohmann::json::parse(json_str);
        auto plan = translator.translate(j);
        executor.execute(plan.get());
        printSeparator();
    }

    // 7️⃣ Select after Delete
    {
        std::string json_str = R"({
            "type": "Select",
            "table_name": "users",
            "columns": ["name","age"]
        })";
        auto j = nlohmann::json::parse(json_str);
        auto plan = translator.translate(j);
        std::cout << "[Test] Select after Delete:" << std::endl;
        executor.execute(plan.get());
        printSeparator();
    }

    std::cout << "Executor Full Test finished." << std::endl;
    return 0;
}
