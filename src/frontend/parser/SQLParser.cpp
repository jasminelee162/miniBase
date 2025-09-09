/**
 * 输入 "SELECT * FROM student;"
 *  生成一个 PlanNode（type=SeqScan, table_name=student
 */

#include "SQLParser.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <iostream>

PlanNode *SQLParser::parse(const std::string &sql)
{
    std::string s = sql;
    std::transform(s.begin(), s.end(), s.begin(), ::toupper);

    PlanNode *node = new PlanNode();

    if (s.find("CREATE TABLE") == 0)
    {
        node->type = PlanType::CreateTable;
        size_t pos = s.find("TABLE") + 6;
        node->table_name = sql.substr(pos);
    }
    else if (s.find("INSERT INTO") == 0)
    {
        node->type = PlanType::Insert;
        size_t pos = s.find("INTO") + 5;
        node->table_name = sql.substr(pos);
    }
    else if (s.find("SELECT") == 0)
    {
        node->type = PlanType::SeqScan;
        size_t pos = s.find("FROM") + 5;
        node->table_name = sql.substr(pos);
    }
    else if (s.find("DELETE FROM") == 0)
    {
        node->type = PlanType::Delete;
        size_t pos = s.find("FROM") + 5;
        node->table_name = sql.substr(pos);
    }
    else
    {
        std::cerr << "Unsupported SQL: " << sql << std::endl;
    }

    return node;
}
