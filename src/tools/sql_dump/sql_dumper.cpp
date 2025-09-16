#include "sql_dumper.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include "../../engine/operators/Row.h" // 用于 Row::Deserialize

namespace minidb
{

    SQLDumper::SQLDumper(Catalog *catalog, StorageEngine *engine)
        : catalog_(catalog), engine_(engine) {}

    bool SQLDumper::DumpToFile(const std::string &filename, DumpOption option)
    {
        std::ofstream ofs(filename);
        if (!ofs.is_open())
            return false;
        ofs << DumpToString(option);
        ofs.close();
        return true;
    }

    std::string SQLDumper::DumpToString(DumpOption option)
    {
        std::ostringstream oss;
        auto tables = catalog_->GetAllTables(); // 你需要在 Catalog 补充这个方法（已经补充）

        for (const auto &table_name : tables)
        {
            TableSchema schema = catalog_->GetTable(table_name);

            if (option == DumpOption::StructureOnly || option == DumpOption::StructureAndData)
            {
                oss << DumpTableSchema(schema) << ";\n\n";
            }
            if (option == DumpOption::DataOnly || option == DumpOption::StructureAndData)
            {
                oss << DumpTableData(schema) << "\n";
            }
        }
        return oss.str();
    }

    std::string SQLDumper::DumpTableSchema(const TableSchema &schema)
    {
        std::ostringstream oss;
        oss << "CREATE TABLE " << schema.table_name << " (";
        for (size_t i = 0; i < schema.columns.size(); i++)
        {
            const auto &col = schema.columns[i];
            oss << col.name << " " << col.type;
            if (col.type == "VARCHAR" || col.type == "CHAR")
            {
                oss << "(" << col.length << ")";
            }
            if (i + 1 < schema.columns.size())
                oss << ", ";
        }
        oss << ")";
        return oss.str();
    }

    std::string SQLDumper::DumpTableData(const TableSchema &schema)
    {
        std::ostringstream oss;
        if (schema.first_page_id == INVALID_PAGE_ID)
            return "";

        // 遍历页链
        auto pages = engine_->GetPageChain(schema.first_page_id);
        for (auto *page : pages)
        {
            auto records = engine_->GetPageRecords(page);
            for (auto &rec : records)
            {
                // 这里你需要用 Row::Deserialize 还原一条记录
                Row row = Row::Deserialize(
                    reinterpret_cast<const unsigned char *>(rec.first),
                    rec.second,
                    schema);

                oss << "INSERT INTO " << schema.table_name << " VALUES(";
                for (size_t i = 0; i < row.columns.size(); i++)
                {
                    oss << "'" << row.columns[i].value << "'";
                    if (i + 1 < row.columns.size())
                        oss << ", ";
                }
                oss << ");\n";
            }
        }
        return oss.str();
    }

} // namespace minidb
