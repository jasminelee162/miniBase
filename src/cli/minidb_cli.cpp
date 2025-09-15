#include "../sql_compiler/lexer/lexer.h"
#include "../sql_compiler/parser/parser.h"
#include "../sql_compiler/parser/ast_json_serializer.h"
#include "../sql_compiler/semantic/semantic_analyzer.h"
#include "../frontend/translator/json_to_plan.h"
#include "../storage/storage_engine.h"
#include "../engine/executor/executor.h"
#include "../catalog/catalog.h"
#include "../util/table_utils.h"
#include <iostream>
#include <string>
#include <memory>
#include <fstream>
#include <algorithm>
#include <vector>
#include <cctype>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

static void print_help()
{
    std::cout << "MiniDB CLI\n"
              << "Usage: minidb_cli [--exec|--json] [--db <path>]\n"
              << "Commands:\n"
              << "  .help           Show this help\n"
              << "  .exit           Quit\n"
              << "Enter SQL terminated by ';' to run.\n";
}

// 简易编辑距离（Levenshtein），用于纠正首个关键词的常见拼写错误
static int edit_distance(const std::string &a, const std::string &b)
{
    const size_t n = a.size(), m = b.size();
    if (n == 0) return (int)m;
    if (m == 0) return (int)n;
    std::vector<int> prevRow(m + 1), curRow(m + 1);
    for (size_t j = 0; j <= m; ++j) prevRow[j] = (int)j;
    for (size_t i = 1; i <= n; ++i)
    {
        curRow[0] = (int)i;
        for (size_t j = 1; j <= m; ++j)
        {
            int cost = (std::tolower((unsigned char)a[i - 1]) == std::tolower((unsigned char)b[j - 1])) ? 0 : 1;
            int delCost = prevRow[j] + 1;        // 删除 a[i-1]
            int insCost = curRow[j - 1] + 1;     // 插入 b[j-1]
            int subCost = prevRow[j - 1] + cost; // 置换
            int best = delCost;
            if (insCost < best) best = insCost;
            if (subCost < best) best = subCost;
            curRow[j] = best;
        }
        std::swap(prevRow, curRow);
    }
    return prevRow[m];
}

static std::string autocorrect_leading_keyword(const std::string &sql)
{
    static const char* keywords[] = {"select","insert","update","delete","create","drop","show"};
    // 提取首词
    size_t i = 0; while (i < sql.size() && std::isspace((unsigned char)sql[i])) ++i;
    size_t j = i; while (j < sql.size() && std::isalpha((unsigned char)sql[j])) ++j;
    if (j <= i) return sql;
    std::string head = sql.substr(i, j - i);
    int bestDist = 2; // 仅允许距离<=1的自动纠正
    const char* best = nullptr;
    for (auto k : keywords)
    {
        int d = edit_distance(head, k);
        if (d < bestDist)
        {
            bestDist = d; best = k;
            if (d == 0) break;
        }
    }
    if (best && bestDist == 1)
    {
        std::string corrected = sql;
        corrected.replace(i, j - i, best);
        std::cerr << "[Hint] 已将首个关键词从 '" << head << "' 更正为 '" << best << "'\n";
        return corrected;
    }
    return sql;
}

// 判断是否可以在无分号情况下结束：当括号配平且当前输入行为空
static bool can_terminate_without_semicolon(const std::string &buffer, const std::string &line)
{
    if (!line.empty()) return false;
    int balance = 0; bool in_str = false; char quote = 0;
    for (size_t i = 0; i < buffer.size(); ++i)
    {
        char c = buffer[i];
        if (in_str)
        {
            if (c == quote)
            {
                // 处理转义
                if (i + 1 < buffer.size() && buffer[i + 1] == quote) { ++i; continue; }
                in_str = false; quote = 0;
            }
            continue;
        }
        if (c == '\'' || c == '"') { in_str = true; quote = c; continue; }
        if (c == '(') ++balance; else if (c == ')') --balance;
    }
    return balance == 0 && !buffer.empty();
}

int main(int argc, char **argv)
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    bool doExec = true;
    bool outputJsonOnly = false;
    std::string dbFile = "data/mini.db";

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--exec")
        {
            doExec = true;
            outputJsonOnly = false;
        }
        else if (arg == "--json")
        {
            outputJsonOnly = true;
            doExec = false;
        }
        else if (arg == "--db" && i + 1 < argc)
        {
            dbFile = argv[++i];
        }
        else if (arg == "-h" || arg == "--help")
        {
            print_help();
            return 0;
        }
        else
        {
            std::cerr << "Unknown argument: " << arg << std::endl;
            print_help();
            return 1;
        }
    }

    std::shared_ptr<minidb::StorageEngine> se;
    std::shared_ptr<minidb::Catalog> catalog;
    std::unique_ptr<minidb::Executor> exec;
    if (doExec)
    {
        se = std::make_shared<minidb::StorageEngine>(dbFile, 16);
        catalog = std::make_shared<minidb::Catalog>(se.get());
        exec = std::make_unique<minidb::Executor>(se);
        exec->SetCatalog(catalog);
    }

    std::cout << "MiniDB CLI ready. Type .help for help.\n";

    std::string buffer;
    buffer.reserve(1024);

    while (true)
    {
        std::cout << ">> " << std::flush;
        std::string line;
        if (!std::getline(std::cin, line))
            break;
        if (line == ".exit")
            break;
        if (line == ".help")
        {
            print_help();
            continue;
        }
        if (line.empty())
            continue;

        buffer += line;
        if (buffer.find(';') == std::string::npos)
        {
            if (can_terminate_without_semicolon(buffer, line))
            {
                // 允许在空行且括号配平时，无分号结束
            }
            else
            {
                buffer += '\n';
                continue;
            }
        }

        try
        {
            // 首关键词自动纠错（距离<=1）
            std::string toParse = autocorrect_leading_keyword(buffer);
            Lexer l(toParse);
            auto tokens = l.tokenize();
            Parser p(tokens);
            auto stmt = p.parse();

            // 语义检查
            try
            {
                SemanticAnalyzer sem;
                sem.setCatalog(catalog.get());
                sem.analyze(stmt.get());
            }
            catch (const std::exception &e)
            {
                std::cerr << "[Semantic][ERROR] " << e.what() << std::endl;
                // 语义错误为致命错误：跳过执行阶段
                buffer.clear();
                continue;
            }

            auto j = ASTJson::toJson(stmt.get());
            std::cout << "astjson: " << j.dump(2) << std::endl;

            if (outputJsonOnly)
            {
                std::cout << "只有astjson: " << j.dump(2) << std::endl;
            }
            else if (doExec)
            {
                auto plan = JsonToPlan::translate(j);

                // 执行 Plan
                auto results = exec->execute(plan.get());
                std::cout << "[OK] executed." << std::endl;
                TablePrinter::printResults(results, " ");

                // ✅ 强制刷新 Catalog & StorageEngine，保证持久化
                catalog->SaveToStorage();
                // se->flush();
            }
        }
        catch (const ParseError &e)
        {
            std::cerr << "[Parser][ERROR] [minidb_cli] " << e.what()
                      << " at (" << e.getLine() << "," << e.getColumn() << ")" << std::endl;
        }
        catch (const std::exception &e)
        {
            std::cerr << "[ERROR] " << e.what() << std::endl;
        }

        buffer.clear();
    }

    std::cout << "Bye." << std::endl;
    return 0;
}
