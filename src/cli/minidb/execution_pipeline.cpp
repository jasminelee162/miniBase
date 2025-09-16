#include "execution_pipeline.h"

#include "../sql_compiler/lexer/lexer.h"
#include "../sql_compiler/parser/parser.h"
#include "../sql_compiler/parser/ast_json_serializer.h"
#include "../sql_compiler/semantic/semantic_analyzer.h"
#include "../frontend/translator/json_to_plan.h"
#include "../util/table_utils.h"
#include "../catalog/catalog.h"
#include "../engine/executor/executor.h"
#include "../storage/storage_engine.h"

#include <iostream>
#include <sstream>
#include "cli_helpers.h"

namespace minidb {
namespace cli {

bool execute_sql_pipeline(
    const std::string &sql,
    Catalog *catalog,
    Executor *executor,
    StorageEngine * /*storageEngine*/,
    bool outputJsonOnly)
{
    try
    {
        Lexer l(sql);
        auto tokens = l.tokenize();
        Parser p(tokens);
        auto stmt = p.parse();

        try
        {
            SemanticAnalyzer sem;
            sem.setCatalog(catalog);
            sem.analyze(stmt.get());
        }
        catch (const std::exception &e)
        {
            std::cerr << "[Semantic][ERROR] " << e.what() << std::endl;
            return false;
        }

        auto j = ASTJson::toJson(stmt.get());
        {
            std::ostringstream oss;
            oss << "AST JSON:\n" << j.dump(2);
            log_debug(oss.str());
        }

        if (outputJsonOnly)
        {
            // 仅当用户显式请求 --json 时，才在终端输出 JSON
            std::cout << j.dump(2) << std::endl;
            return true;
        }

        auto plan = JsonToPlan::translate(j);
        auto results = executor->execute(plan.get());
        log_info("execution finished successfully");
        // 打印结果表格（仅针对 SELECT/SHOW 等返回行的命令）
        TablePrinter::printResults(results, j.value("type", "SELECT"));
        // 打印操作摘要（非查询类）
        if (results.empty()) {
            std::string summary = executor->TakeOperationSummary();
            if (!summary.empty()) {
                std::cout << summary << std::endl;
            }
        }

        // ✅ 持久化 catalog（原逻辑保留）
        catalog->SaveToStorage();
        return true;
    }
    catch (const ParseError &e)
    {
        log_error(std::string("[Parser][ERROR] ") + e.what());
        std::cerr << "[Parser][ERROR] [minidb_cli] " << e.what()
                  << " at (" << e.getLine() << "," << e.getColumn() << ")" << std::endl;
        return false;
    }
    catch (const std::exception &e)
    {
        log_error(std::string("[ERROR] ") + e.what());
        std::cerr << "[ERROR] " << e.what() << std::endl;
        return false;
    }
}

} // namespace cli
} // namespace minidb


