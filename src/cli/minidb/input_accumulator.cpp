#include "input_accumulator.h"

#include "cli_helpers.h"
#include "../util/sql_input_utils.h"

namespace minidb {
namespace cli {

void InputAccumulator::append_line(const std::string &line)
{
    if (line.empty())
    {
        if (ready())
        {
            // 空行作为软终止仅在已满足条件时生效
        }
        else
        {
            buffer_ += '\n';
        }
        return;
    }
    buffer_ += line;
    int beginCount = count_substring(buffer_, "BEGIN");
    int endCount = count_substring(buffer_, "END");
    bool hasUnclosedBlock = beginCount > endCount;
    if (buffer_.find(';') == std::string::npos || hasUnclosedBlock)
    {
        if (minidb::cliutil::can_terminate_without_semicolon(buffer_, line))
        {
            // 允许无分号结束
        }
        else
        {
            buffer_ += '\n';
        }
    }
}

bool InputAccumulator::ready() const
{
    int beginCount = count_substring(buffer_, "BEGIN");
    int endCount = count_substring(buffer_, "END");
    bool hasUnclosedBlock = beginCount > endCount;
    if (buffer_.empty()) return false;
    if (buffer_.find(';') != std::string::npos && !hasUnclosedBlock) return true;
    // can_terminate_without_semicolon 需要最后一行上下文，这里简化判断：
    return false;
}

std::string InputAccumulator::take()
{
    std::string out;
    out.swap(buffer_);
    return out;
}

} // namespace cli
} // namespace minidb


