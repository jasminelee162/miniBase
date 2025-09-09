#include "../../src/catalog/catalog.h"
#include <iostream>
#include <cassert>
#include <filesystem>

using namespace minidb;

int main()
{

    std::cout << "Running CatalogTest..." << std::endl;
    const std::string catalog_file1 = "data/catalog.dat";
    const std::string catalog_file = R"(E:\project\miniBase\data\catalog.dat)";
    std::cout << "Current working directory: " << std::filesystem::current_path() << std::endl;
    // 为测试，先删除旧文件
    // if (std::filesystem::exists(catalog_file))
    // {
    //     std::filesystem::remove(catalog_file);
    // }

    std::cout << "Using catalog file: " << catalog_file << std::endl;
    Catalog catalog(catalog_file);

    std::cout << "Catalog initialized." << std::endl;
    // 测试建表
    catalog.CreateTable("students", {"id", "name", "age"});
    std::cout << "第一步" << std::endl;
    catalog.CreateTable("courses", {"id", "title"});
    std::cout << "[CatalogTest] 创建表成功 ✅" << std::endl;

    // 测试 HasTable
    assert(catalog.HasTable("students"));
    assert(catalog.HasTable("courses"));
    assert(!catalog.HasTable("teachers"));
    std::cout << "[CatalogTest] HasTable 测试通过 ✅" << std::endl;

    // 测试 GetTable
    auto students_schema = catalog.GetTable("students");
    assert(students_schema.table_name == "students");
    assert(students_schema.columns.size() == 3);
    assert(students_schema.columns[0].name == "id");
    assert(students_schema.columns[1].name == "name");
    assert(students_schema.columns[2].name == "age");
    std::cout << "[CatalogTest] GetTable 测试通过 ✅" << std::endl;

    auto courses_schema = catalog.GetTable("courses");
    assert(courses_schema.columns[1].name == "title");

    std::cout << "[CatalogTest] 所有测试通过 ✅" << std::endl;

    return 0;
}
