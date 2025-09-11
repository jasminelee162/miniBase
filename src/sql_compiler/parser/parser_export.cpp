#include "parser.h"
#include "../lexer/lexer.h"
#include "ast_json_serializer.h"
#include "../../frontend/translator/json_to_plan.h"
#include "../../storage/storage_engine.h"
#include "../../engine/executor/executor.h"
#include "../../catalog/catalog.h"
#include <fstream>
#include <iostream>
#include <memory>

int main(int argc, char** argv)
{
    std::string sql = "SELECT id,name FROM student WHERE age > 18;";
    
    // 支持从命令行参数读取SQL
    if (argc > 1 && std::string(argv[1]) != "--exec" && std::string(argv[1]) != "--db" && std::string(argv[1]) != "--catalog") {
        sql = argv[1];
    }
    
    // Tokenize
    Lexer l(sql);
    auto tokens = l.tokenize();

    // Parse
    Parser p(tokens);
    auto stmt = p.parse();

    bool doExec = false;
    std::string dbFile = "data/mini.db";
    std::string catalogFile = "catalog.dat";
    std::string outputFile = "out/last_plan.json";
    
    for (int i=1;i<argc;i++){
      if (std::string(argv[i])=="--exec") doExec=true;
      else if (std::string(argv[i])=="--db" && i+1<argc) dbFile=argv[++i];
      else if (std::string(argv[i])=="--catalog" && i+1<argc) catalogFile=argv[++i];
      else if (std::string(argv[i])=="--output" && i+1<argc) outputFile=argv[++i];
    }
    
    auto j = ASTJson::toJson(stmt.get());
    
    if (!doExec) { 
        // 输出JSON到文件
        std::ofstream outFile(outputFile);
        if (outFile.is_open()) {
            outFile << j.dump(2) << std::endl;
            outFile.close();
            std::cout << "JSON plan written to: " << outputFile << std::endl;
        } else {
            std::cerr << "Failed to open output file: " << outputFile << std::endl;
            return 1;
        }
        return 0; 
    }

    // 执行模式：translate -> execute
    try {
        auto plan = JsonToPlan::translate(j);
        auto se = std::make_shared<minidb::StorageEngine>(dbFile, 16);
        minidb::Executor exec(se);
        auto catalog = std::make_shared<minidb::Catalog>(catalogFile);
        exec.SetCatalog(catalog);

        std::cout << "Executing plan..." << std::endl;
        exec.execute(plan.get());
        std::cout << "Execution completed successfully." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Execution error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
