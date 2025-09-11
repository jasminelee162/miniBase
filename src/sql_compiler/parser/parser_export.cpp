#include "parser.h"
#include "../lexer/lexer.h"
#include "ast_json_serializer.h"
#include <fstream>
#include <iostream>

int main(int argc, char** argv)
{
    std::string sql = "SELECT id,name FROM student WHERE age > 18;";
    // Tokenize
    Lexer l(sql);
    auto tokens = l.tokenize();

    // Parse
    Parser p(tokens);
    auto stmt = p.parse();

    // Serialize AST to JSON (DB-facing format)
    auto j = ASTJson::toJson(stmt.get());
    std::ofstream ofs("out/last_plan.json");
    ofs << j.dump(2);
    ofs.close();

    std::cout << "Wrote out/last_plan.json" << std::endl;
    return 0;
}
