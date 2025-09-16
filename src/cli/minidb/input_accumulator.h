#pragma once

#include <string>

namespace minidb {
namespace cli {

// 管理多行 SQL 输入，直到检测到完整语句（分号或 BEGIN/END 配平且空行终止）
class InputAccumulator {
public:
    void append_line(const std::string &line);
    bool ready() const;            // 是否可提交
    std::string take();            // 取出并清空缓冲

private:
    std::string buffer_;
};

} // namespace cli
} // namespace minidb


