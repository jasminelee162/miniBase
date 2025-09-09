#include "logger.h"

Logger::Logger(const std::string &filename)
    : log_file(filename, std::ios::app) {}

void Logger::log(const std::string &msg)
{
    log_file << msg << std::endl;
}
