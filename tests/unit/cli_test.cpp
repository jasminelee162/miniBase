#include "../../src/sql_compiler/lexer/lexer.h"
#include "../../src/sql_compiler/parser/parser.h"
#include "../../src/sql_compiler/parser/ast_json_serializer.h"
#include "../../src/sql_compiler/semantic/semantic_analyzer.h"
#include "../../src/frontend/translator/json_to_plan.h"
#include "../../src/storage/storage_engine.h"
#include "../../src/engine/executor/executor.h"
#include "../../src/catalog/catalog.h"
#include "../simple_test_framework.h"
#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <sstream>
#ifdef _WIN32
#include <windows.h>
#endif

using namespace SimpleTest;

class CLITestHelper {
private:
    std::shared_ptr<minidb::StorageEngine> se_;
    std::shared_ptr<minidb::Catalog> catalog_;
    std::unique_ptr<minidb::Executor> exec_;
    std::stringstream output_;
    std::stringstream error_;

public:
    CLITestHelper(const std::string& dbFile = "test_cli.db") {
        // 创建测试数据库
        se_ = std::make_shared<minidb::StorageEngine>(dbFile, 10);
        catalog_ = std::make_shared<minidb::Catalog>(se_.get());
        exec_ = std::make_unique<minidb::Executor>(se_);
        exec_->SetCatalog(catalog_);
    }

    // 执行SQL命令并捕获输出
    bool executeSQL(const std::string& sql) {
        try {
            output_.str("");
            error_.str("");

            Lexer l(sql);
            auto tokens = l.tokenize();
            Parser p(tokens);
            auto stmt = p.parse();

            // 语义分析
            try {
                SemanticAnalyzer sem;
                sem.setCatalog(catalog_.get());
                sem.analyze(stmt.get());
            } catch (const std::exception &e) {
                error_ << "[Semantic][WARN] " << e.what();
            }

            auto j = ASTJson::toJson(stmt.get());
            auto plan = JsonToPlan::translate(j);
            exec_->execute(plan.get());
            
            output_ << "[OK] executed.";
            return true;
        } catch (const ParseError &e) {
            error_ << "[Parser][ERROR] " << e.what() << " at (" << e.getLine() << "," << e.getColumn() << ")";
            return false;
        } catch (const std::exception &e) {
            error_ << "[ERROR] " << e.what();
            return false;
        }
    }

    std::string getOutput() const { return output_.str(); }
    std::string getError() const { return error_.str(); }
    bool hasError() const { return !error_.str().empty(); }
};

// 测试用例
void testCreateTable() {
    CLITestHelper helper;
    
    std::string sql = "CREATE TABLE students (id INT, name VARCHAR(50), age INT, grade VARCHAR(10));";
    bool success = helper.executeSQL(sql);
    
    ASSERT_TRUE(success);
    ASSERT_FALSE(helper.hasError());
    std::cout << "Created table successfully" << std::endl;
}

void testInsertData() {
    CLITestHelper helper;
    
    // 先创建表
    helper.executeSQL("CREATE TABLE students (id INT, name VARCHAR(50), age INT, grade VARCHAR(10));");
    
    // 插入数据
    std::vector<std::string> insertStatements = {
        "INSERT INTO students VALUES (1, 'Alice', 20, 'A');",
        "INSERT INTO students VALUES (2, 'Bob', 19, 'B');",
        "INSERT INTO students VALUES (3, 'Charlie', 21, 'A');",
        "INSERT INTO students VALUES (4, 'David', 18, 'C');",
        "INSERT INTO students VALUES (5, 'Eva', 22, 'B');"
    };
    
    for (const auto& sql : insertStatements) {
        bool success = helper.executeSQL(sql);
        ASSERT_TRUE(success);
        ASSERT_FALSE(helper.hasError());
    }
    
    std::cout << "Inserted " << insertStatements.size() << " records successfully" << std::endl;
}

void testSelectAll() {
    CLITestHelper helper;
    
    // 准备数据
    helper.executeSQL("CREATE TABLE students (id INT, name VARCHAR(50), age INT, grade VARCHAR(10));");
    helper.executeSQL("INSERT INTO students VALUES (1, 'Alice', 20, 'A');");
    helper.executeSQL("INSERT INTO students VALUES (2, 'Bob', 19, 'B');");
    helper.executeSQL("INSERT INTO students VALUES (3, 'Charlie', 21, 'A');");
    
    // 查询所有数据
    bool success = helper.executeSQL("SELECT * FROM students;");
    ASSERT_TRUE(success);
    ASSERT_FALSE(helper.hasError());
    
    std::cout << "Selected all records successfully" << std::endl;
}

void testSelectWithCondition() {
    CLITestHelper helper;
    
    // 准备数据
    helper.executeSQL("CREATE TABLE students (id INT, name VARCHAR(50), age INT, grade VARCHAR(10));");
    helper.executeSQL("INSERT INTO students VALUES (1, 'Alice', 20, 'A');");
    helper.executeSQL("INSERT INTO students VALUES (2, 'Bob', 19, 'B');");
    helper.executeSQL("INSERT INTO students VALUES (3, 'Charlie', 21, 'A');");
    helper.executeSQL("INSERT INTO students VALUES (4, 'David', 18, 'C');");
    
    // 条件查询
    bool success = helper.executeSQL("SELECT * FROM students WHERE age > 19;");
    ASSERT_TRUE(success);
    ASSERT_FALSE(helper.hasError());
    
    std::cout << "Selected records with condition successfully" << std::endl;
}

void testUpdateData() {
    CLITestHelper helper;
    
    // 准备数据
    helper.executeSQL("CREATE TABLE students (id INT, name VARCHAR(50), age INT, grade VARCHAR(10));");
    helper.executeSQL("INSERT INTO students VALUES (1, 'Alice', 20, 'A');");
    helper.executeSQL("INSERT INTO students VALUES (2, 'Bob', 19, 'B');");
    
    // 更新数据
    bool success = helper.executeSQL("UPDATE students SET grade = 'A+' WHERE name = 'Alice';");
    ASSERT_TRUE(success);
    ASSERT_FALSE(helper.hasError());
    
    std::cout << "Updated record successfully" << std::endl;
}

void testDeleteData() {
    CLITestHelper helper;
    
    // 准备数据
    helper.executeSQL("CREATE TABLE students (id INT, name VARCHAR(50), age INT, grade VARCHAR(10));");
    helper.executeSQL("INSERT INTO students VALUES (1, 'Alice', 20, 'A');");
    helper.executeSQL("INSERT INTO students VALUES (2, 'Bob', 19, 'B');");
    helper.executeSQL("INSERT INTO students VALUES (3, 'Charlie', 21, 'A');");
    
    // 删除数据
    bool success = helper.executeSQL("DELETE FROM students WHERE age < 20;");
    ASSERT_TRUE(success);
    ASSERT_FALSE(helper.hasError());
    
    std::cout << "Deleted records successfully" << std::endl;
}

void testComplexQuery() {
    CLITestHelper helper;
    
    // 准备数据
    helper.executeSQL("CREATE TABLE students (id INT, name VARCHAR(50), age INT, grade VARCHAR(10));");
    helper.executeSQL("INSERT INTO students VALUES (1, 'Alice', 20, 'A');");
    helper.executeSQL("INSERT INTO students VALUES (2, 'Bob', 19, 'B');");
    helper.executeSQL("INSERT INTO students VALUES (3, 'Charlie', 21, 'A');");
    helper.executeSQL("INSERT INTO students VALUES (4, 'David', 18, 'C');");
    helper.executeSQL("INSERT INTO students VALUES (5, 'Eva', 22, 'B');");
    
    // 复杂查询：选择特定列和条件
    bool success = helper.executeSQL("SELECT name, age FROM students WHERE grade = 'A' AND age >= 20;");
    ASSERT_TRUE(success);
    ASSERT_FALSE(helper.hasError());
    
    std::cout << "Complex query executed successfully" << std::endl;
}

void testErrorHandling() {
    CLITestHelper helper;
    
    // 测试语法错误
    bool success = helper.executeSQL("SELCT * FROM students;");  // 故意拼写错误
    ASSERT_FALSE(success);
    ASSERT_TRUE(helper.hasError());
    
    std::cout << "Error handling test passed" << std::endl;
}

void testCompleteWorkflow() {
    CLITestHelper helper;
    
    std::cout << "\n=== Complete Workflow Test ===" << std::endl;
    
    // 1. 创建表
    std::cout << "1. Creating table..." << std::endl;
    ASSERT_TRUE(helper.executeSQL("CREATE TABLE teachers (id INT, name VARCHAR(50), subject VARCHAR(30), experience INT);"));
    
    // 2. 插入数据
    std::cout << "2. Inserting data..." << std::endl;
    std::vector<std::string> teachers = {
        "INSERT INTO teachers VALUES (101, 'Dr. Smith', 'Mathematics', 10);",
        "INSERT INTO teachers VALUES (102, 'Prof. Johnson', 'Physics', 15);",
        "INSERT INTO teachers VALUES (103, 'Ms. Brown', 'Chemistry', 8);",
        "INSERT INTO teachers VALUES (104, 'Dr. Wilson', 'Biology', 12);",
        "INSERT INTO teachers VALUES (105, 'Mr. Davis', 'English', 5);"
    };
    
    for (const auto& sql : teachers) {
        ASSERT_TRUE(helper.executeSQL(sql));
    }
    
    // 3. 查询所有数据
    std::cout << "3. Selecting all data..." << std::endl;
    ASSERT_TRUE(helper.executeSQL("SELECT * FROM teachers;"));
    
    // 4. 条件查询
    std::cout << "4. Querying experienced teachers..." << std::endl;
    ASSERT_TRUE(helper.executeSQL("SELECT name, subject FROM teachers WHERE experience > 10;"));
    
    // 5. 更新数据
    std::cout << "5. Updating teacher experience..." << std::endl;
    ASSERT_TRUE(helper.executeSQL("UPDATE teachers SET experience = 11 WHERE name = 'Dr. Smith';"));
    
    // 6. 验证更新
    std::cout << "6. Verifying update..." << std::endl;
    ASSERT_TRUE(helper.executeSQL("SELECT * FROM teachers WHERE name = 'Dr. Smith';"));
    
    // 7. 删除数据
    std::cout << "7. Deleting junior teacher..." << std::endl;
    ASSERT_TRUE(helper.executeSQL("DELETE FROM teachers WHERE experience < 6;"));
    
    // 8. 最终查询
    std::cout << "8. Final query..." << std::endl;
    ASSERT_TRUE(helper.executeSQL("SELECT * FROM teachers ORDER BY experience DESC;"));
    
    std::cout << "Complete workflow test passed!" << std::endl;
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    TestSuite suite;
    
    // 添加测试用例
    suite.addTest("Create Table", testCreateTable);
    suite.addTest("Insert Data", testInsertData);
    suite.addTest("Select All", testSelectAll);
    suite.addTest("Select With Condition", testSelectWithCondition);
    suite.addTest("Update Data", testUpdateData);
    suite.addTest("Delete Data", testDeleteData);
    suite.addTest("Complex Query", testComplexQuery);
    suite.addTest("Error Handling", testErrorHandling);
    suite.addTest("Complete Workflow", testCompleteWorkflow);
    
    // 运行所有测试
    suite.runAll();
    
    return 0;
}
