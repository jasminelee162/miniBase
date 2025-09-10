#pragma once
#include <string>
#include <fstream>
#include <mutex>

class Logger
{
public:
    enum class Level { DEBUG = 0, INFO = 1, WARN = 2, ERROR = 3 };

    explicit Logger(const std::string &filename);
    ~Logger();

    // default info-level log
    void log(const std::string &msg);
    // level-specific log
    void log(Level level, const std::string &msg);

    void setLevel(Level level);
    Level getLevel() const;

private:
    std::ofstream log_file;
    mutable std::mutex mu;
    Level current_level = Level::INFO;

    static const char* levelToString(Level lvl);
    static std::string timeStamp();
};
