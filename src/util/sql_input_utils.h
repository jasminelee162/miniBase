#pragma once

#include <string>
#include <vector>
#include <cctype>
#include <algorithm>
#include <iostream>

namespace minidb {
namespace cliutil {

// 简易编辑距离（Levenshtein），用于纠正首个关键词的常见拼写错误
inline int edit_distance(const std::string &a, const std::string &b)
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

// 首关键词自动纠错（距离<=1）
inline std::string autocorrect_leading_keyword(const std::string &sql)
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
inline bool can_terminate_without_semicolon(const std::string &buffer, const std::string &line)
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
                // 处理转义（连写两个引号视为转义）
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

} // namespace cliutil
} // namespace minidb


