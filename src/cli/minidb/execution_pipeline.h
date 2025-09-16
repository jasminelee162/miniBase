#pragma once

#include <memory>
#include <string>

namespace minidb {

class Catalog;
class Executor;
class StorageEngine;

namespace cli {

// 单次执行 SQL 的完整管线：解析→语义→翻译→执行→打印→持久化
// 返回是否成功执行
bool execute_sql_pipeline(
    const std::string &sql,
    Catalog *catalog,
    Executor *executor,
    StorageEngine *storageEngine,
    bool outputJsonOnly);

} // namespace cli
} // namespace minidb


