
// ===== table_utils.h =====
#ifndef TABLE_UTILS_H
#define TABLE_UTILS_H

#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include "../engine/operators/Row.h"

class TablePrinter {
public:
    static void printResults(const std::vector<Row>& rows, const std::string& queryType = "SELECT") {
        if (rows.empty()) {
            std::cout << "Empty result set." << std::endl;
            return;
        }
        
        // 获取所有列名
        std::vector<std::string> headers;
        for (const auto& col : rows[0].columns) {
            headers.push_back(col.col_name);
        }
        
        // 计算每列的最大宽度
        std::vector<size_t> columnWidths;
        for (const auto& header : headers) {
            columnWidths.push_back(header.length());
        }
        
        // 遍历所有行，计算每列的最大宽度
        for (const auto& row : rows) {
            for (size_t i = 0; i < row.columns.size() && i < columnWidths.size(); ++i) {
                columnWidths[i] = std::max(columnWidths[i], row.columns[i].value.length());
            }
        }
        
        // 确保最小宽度为 8
        for (auto& width : columnWidths) {
            width = std::max(width, static_cast<size_t>(8));
        }
        
        // 打印查询类型
        std::cout << "\n===== " << queryType << " RESULT =====" << std::endl;
        
        // 打印表头边框
        printBorder(columnWidths);
        
        // 打印表头
        std::cout << "|";
        for (size_t i = 0; i < headers.size(); ++i) {
            std::cout << " " << std::left << std::setw(columnWidths[i]) << headers[i] << " |";
        }
        std::cout << std::endl;
        
        // 打印表头下方边框
        printBorder(columnWidths);
        
        // 打印数据行
        for (const auto& row : rows) {
            std::cout << "|";
            for (size_t i = 0; i < row.columns.size() && i < columnWidths.size(); ++i) {
                std::cout << " " << std::left << std::setw(columnWidths[i]) << row.columns[i].value << " |";
            }
            std::cout << std::endl;
        }
        
        // 打印底部边框
        printBorder(columnWidths);
        
        // 打印行数统计
        std::cout << "(" << rows.size() << " row" << (rows.size() != 1 ? "s" : "") << " returned)" << std::endl;
        std::cout << std::endl;
    }

private:
    static void printBorder(const std::vector<size_t>& columnWidths) {
        std::cout << "+";
        for (const auto& width : columnWidths) {
            std::cout << std::string(width + 2, '-') << "+";
        }
        std::cout << std::endl;
    }
};

#endif