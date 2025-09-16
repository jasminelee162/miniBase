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
        std::cout << "astjson: " << j.dump(2) << std::endl;

        if (outputJsonOnly)
        {
            std::cout << "只有astjson: " << j.dump(2) << std::endl;
            return true;
        }

        auto plan = JsonToPlan::translate(j);
        auto results = executor->execute(plan.get());
        std::cout << "[OK] executed." << std::endl;
        TablePrinter::printResults(results, " ");

        // ✅ 持久化 catalog（原逻辑保留）
        catalog->SaveToStorage();
        return true;
    }
    catch (const ParseError &e)
    {
        std::cerr << "[Parser][ERROR] [minidb_cli] " << e.what()
                  << " at (" << e.getLine() << "," << e.getColumn() << ")" << std::endl;
        return false;
    }
    catch (const std::exception &e)
    {
        std::cerr << "[ERROR] " << e.what() << std::endl;
        return false;
    }
}

} // namespace cli
} // namespace minidb


