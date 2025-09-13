// tests/unit/storage_end_to_end_demo.cpp
#include "storage/storage_engine.h"
#include "../simple_test_framework.h"
#include "storage/page/page_header.h"
#include <iostream>
#include <vector>
#include <cstring>
#ifdef _WIN32
#include <windows.h>
#endif

using namespace minidb;

static std::string make_db(const char* name) {
#ifdef PROJECT_ROOT_DIR
    return std::string(PROJECT_ROOT_DIR) + std::string("/data/") + name;
#else
    return std::string(name);
#endif
}

void test_end_to_end_storage_flow() {
    #ifdef _WIN32
    // 确保控制台使用UTF-8，避免中文乱码
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    #endif

    std::cout << "[步骤] 打开/创建 .bin 并构造 StorageEngine" << std::endl;
    std::string db = make_db("storage_end_to_end_demo.bin");
    StorageEngine engine(db, 64);

    std::cout << "[步骤] 初始化元数据页（第0页）" << std::endl;
    bool meta_ok = engine.InitializeMetaPage();
    ASSERT_TRUE(meta_ok);

    MetaInfo meta = engine.GetMetaInfo();
    std::cout << "[元数据] magic=0x" << std::hex << meta.magic << std::dec
              << ", 版本=" << meta.version
              << ", 页大小=" << meta.page_size
              << ", 下一页号=" << meta.next_page_id
              << ", 目录根页=" << meta.catalog_root << std::endl;

    std::cout << "[步骤] 创建目录页并设置 catalog_root" << std::endl;
    Page* catalog = engine.CreateCatalogPage();
    ASSERT_NOT_NULL(catalog);
    ASSERT_TRUE(catalog->GetPageType() == PageType::CATALOG_PAGE);
    std::cout << "[目录] 页号=" << catalog->GetPageId() << std::endl;

    MetaInfo meta2 = engine.GetMetaInfo();
    std::cout << "[元数据-更新] 下一页号=" << meta2.next_page_id
              << ", 目录根页=" << meta2.catalog_root << std::endl;

    std::cout << "[步骤] 创建数据页并追加记录" << std::endl;
    page_id_t data_pid = INVALID_PAGE_ID;
    Page* data_page = engine.CreateDataPage(&data_pid);
    ASSERT_NOT_NULL(data_page);
    std::cout << "[数据] 页号=" << data_pid << std::endl;

    const char* rec1 = "Hello, World!";
    const char* rec2 = "MiniBase Storage";
    ASSERT_TRUE(engine.AppendRecordToPage(data_page, rec1, (uint16_t)std::strlen(rec1)));
    ASSERT_TRUE(engine.AppendRecordToPage(data_page, rec2, (uint16_t)std::strlen(rec2)));
    engine.PutPage(data_pid, true);

    // Write minimal catalog: [uint32_t name_len][name_bytes][page_id_t first_data_page]
    {
        const std::string table_name = "users";
        uint32_t name_len = static_cast<uint32_t>(table_name.size());
        std::vector<char> buf;
        buf.resize(sizeof(uint32_t) + name_len + sizeof(page_id_t));
        std::memcpy(buf.data(), &name_len, sizeof(uint32_t));
        std::memcpy(buf.data() + sizeof(uint32_t), table_name.data(), name_len);
        std::memcpy(buf.data() + sizeof(uint32_t) + name_len, &data_pid, sizeof(page_id_t));
        ASSERT_TRUE(engine.UpdateCatalogData(CatalogData(buf)));
        engine.PutPage(catalog->GetPageId(), true);
        std::cout << "[目录] 已写入: 表名='" << table_name << "' 首数据页=" << data_pid << std::endl;
    }

    std::cout << "[步骤] 通过目录页解析首数据页并读取" << std::endl;
    Page* cat_read = engine.GetCatalogPage();
    ASSERT_NOT_NULL(cat_read);
    const char* base = cat_read->GetData() + PAGE_HEADER_SIZE;
    uint32_t name_len2 = 0;
    std::memcpy(&name_len2, base, sizeof(uint32_t));
    std::string name2(base + sizeof(uint32_t), base + sizeof(uint32_t) + name_len2);
    page_id_t first_pid = INVALID_PAGE_ID;
    std::memcpy(&first_pid, base + sizeof(uint32_t) + name_len2, sizeof(page_id_t));
    std::cout << "[目录] 解析得到: 表名='" << name2 << "' 首数据页=" << first_pid << std::endl;

    Page* read_page = engine.GetDataPage(first_pid);
    ASSERT_NOT_NULL(read_page);
    auto recs = engine.GetPageRecords(read_page);
    std::cout << "[读取] 记录条数=" << recs.size() << std::endl;
    ASSERT_EQ((size_t)2, recs.size());
    // Print contents
    for (size_t i = 0; i < recs.size(); ++i) {
        const char* p = reinterpret_cast<const char*>(recs[i].first);
        std::string s(p, p + recs[i].second);
        std::cout << "  [记录] #" << i << " 长度=" << recs[i].second << " 数据='" << s << "'" << std::endl;
    }
    engine.PutPage(read_page->GetPageId(), false);

    std::cout << "[步骤] 执行检查点并关闭" << std::endl;
    engine.Checkpoint();
    engine.Shutdown();
}

int main() {
    SimpleTest::TestSuite suite;
    suite.addTest("StorageEndToEndDemo", test_end_to_end_storage_flow);
    suite.runAll();
    return SimpleTest::TestCase::getFailed() > 0 ? 1 : 0;
}


