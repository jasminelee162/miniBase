#pragma once
#include <string>
#include "../../catalog/catalog.h"
#include "../../storage/storage_engine.h"

namespace minidb
{

    enum class DumpOption
    {
        StructureOnly,
        DataOnly,
        StructureAndData
    };

    class SQLDumper
    {
    public:
        SQLDumper(Catalog *catalog, StorageEngine *engine);

        // 导出到文件
        bool DumpToFile(const std::string &filename, DumpOption option);

        // 导出为字符串（方便测试）
        std::string DumpToString(DumpOption option);

    private:
        std::string DumpTableSchema(const TableSchema &schema);
        std::string DumpTableData(const TableSchema &schema);

        Catalog *catalog_;
        StorageEngine *engine_;
    };

} // namespace minidb
