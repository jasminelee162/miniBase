#pragma once
#include <string>
#include <fstream>

class Logger
{
public:
    Logger(const std::string &filename);
    void log(const std::string &msg);

private:
    std::ofstream log_file;
};
