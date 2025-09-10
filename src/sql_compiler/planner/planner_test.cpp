#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include "../lexer/lexer.h"
#include "../parser/parser.h"
#include "../semantic/semantic_analyzer.h"
#include "planner.h"
#include "plan_printer.h"

// 测试执行计划生成
void testPlanner(const std::string& sql) {
    std::cout << "\n===== Testing SQL: " << sql << " =====\n" << std::endl;
    
    try {
        // 词法分析
        Lexer lexer(sql);
        std::vector<Token> tokens = lexer.tokenize();
        
        std::cout << "Tokens:" << std::endl;
        for (const auto& token : tokens) {
            // std::cout << "  " << token.toString() << std::endl;
        }
        
        // 语法分析
        Parser parser(tokens);
        std::unique_ptr<Statement> stmt = parser.parse();
        
        std::cout << "\nAST:" << std::endl;
        // 这里可以使用ASTPrinter打印AST，但为了简化，我们只打印语句类型
        if (dynamic_cast<CreateTableStatement*>(stmt.get())) {
            std::cout << "  CreateTableStatement" << std::endl;
        } else if (dynamic_cast<InsertStatement*>(stmt.get())) {
            std::cout << "  InsertStatement" << std::endl;
        } else if (dynamic_cast<SelectStatement*>(stmt.get())) {
            std::cout << "  SelectStatement" << std::endl;
        } else if (dynamic_cast<DeleteStatement*>(stmt.get())) {
            std::cout << "  DeleteStatement" << std::endl;
        }
        
        // 语义分析
        SemanticAnalyzer semanticAnalyzer;
        semanticAnalyzer.analyze(stmt.get());
        std::cout << "\nSemantic Analysis: Passed" << std::endl;
        
        // 生成执行计划
        Planner planner;
        std::unique_ptr<PlanNode> plan = planner.plan(stmt.get());
        
        std::cout << "\nExecution Plan:" << std::endl;
        // 打印执行计划
        PlanPrinter printer;
        std::cout << printer.print(plan.get()) << std::endl;
        
    // } catch (const Lexer::LexerError& e) {
    //     std::cout << "Lexer Error: " << e.what() << " at line " << e.getLine() 
    //              << ", column " << e.getColumn() << std::endl;
    // } catch (const ParseError& e) {
    //     std::cout << "Parser Error: " << e.what() << " at token " << e.getToken().toString() << std::endl;
    } catch (const SemanticError& e) {
        std::cout << "Semantic Error: [" << SemanticError::errorTypeToString(e.getType()) 
                 << "] " << e.what() << std::endl;
    } catch (const PlannerError& e) {
        std::cout << "Planner Error: [" << PlannerError::errorTypeToString(e.getType()) 
                 << "] " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
}

int main() {
    // 测试CREATE TABLE语句
    testPlanner("CREATE TABLE student(id INT, name VARCHAR, age INT);");
    
    // 测试INSERT语句
    testPlanner("INSERT INTO student(id, name, age) VALUES (1, 'Alice', 20);");
    
    // 测试SELECT语句
    testPlanner("SELECT id, name FROM student WHERE age > 18;");
    
    // 测试DELETE语句
    testPlanner("DELETE FROM student WHERE id = 1;");
    
    // 测试错误情况
    testPlanner("SELECT * FROM nonexistent_table;"); // 表不存在
    
    return 0;
}