#include "../simple_test_framework.h"
#include "../../src/catalog/catalog.h"
#include "../../src/engine/executor/executor.h"
#include "../../src/auth/auth_service.h"
#include "../../src/sql_compiler/lexer/lexer.h"
#include "../../src/sql_compiler/parser/parser.h"
#include "../../src/sql_compiler/parser/ast_json_serializer.h"
#include "../../src/sql_compiler/semantic/semantic_analyzer.h"
#include "../../src/frontend/translator/json_to_plan.h"

using namespace minidb;
using namespace SimpleTest;

static void runSQL(Catalog* catalog, StorageEngine* se, AuthService* auth, const std::string& sql){
    Lexer lexer(sql);
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    auto stmt = parser.parse();
    SemanticAnalyzer sem; sem.setCatalog(catalog);
    sem.analyze(stmt.get());
    auto j = ASTJson::toJson(stmt.get());
    auto plan = JsonToPlan::translate(j);
    PermissionChecker checker(auth);
    auto exec = std::make_unique<Executor>(catalog, &checker);
    exec->SetAuthService(auth);
    exec->SetStorageEngine(std::shared_ptr<StorageEngine>(se, [](StorageEngine*){}));
    exec->SetCatalog(std::shared_ptr<Catalog>(catalog, [](Catalog*){}));
    exec->execute(plan.get());
}

int main(){
    TestSuite suite;

    suite.addTest("constraint validation: NOT NULL and UNIQUE", [](){
        StorageEngine se("data/test_constraint_validation.db", 64);
        Catalog catalog(&se);
        catalog.LoadFromStorage();
        AuthService auth(&se, &catalog);
        auth.login("root", "root");

        // 解析器不支持 IF EXISTS，使用普通 DROP 并忽略不存在异常
        try { runSQL(&catalog, &se, &auth, "DROP TABLE t1;"); } catch(...) {}
        runSQL(&catalog, &se, &auth, "CREATE TABLE t1(id INT PRIMARY KEY, name VARCHAR(32) NOT NULL, email VARCHAR(64) UNIQUE);");
        runSQL(&catalog, &se, &auth, "INSERT INTO t1(id,name,email) VALUES (1,'A','a@x');");

        // not null violation
        bool notnull_thrown = false;
        try { runSQL(&catalog, &se, &auth, "INSERT INTO t1(id,name,email) VALUES (2,NULL,'b@x');"); } catch (...) { notnull_thrown = true; }
        ASSERT_TRUE(notnull_thrown);

        // unique violation
        bool uniq_thrown = false;
        try { runSQL(&catalog, &se, &auth, "INSERT INTO t1(id,name,email) VALUES (3,'C','a@x');"); } catch (...) { uniq_thrown = true; }
        ASSERT_TRUE(uniq_thrown);
    });

    suite.runAll();
    return TestCase::getFailed();
}


