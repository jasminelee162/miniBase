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

// ---- Global logger (optional) ----
// 初始化一个全局日志器，便于非 CLI 代码写日志而不触及终端
void init_global_logger(const std::string &filename, Logger::Level level);
Logger* get_global_logger();
void global_log(Logger::Level level, const std::string &msg);
inline void global_log_debug(const std::string &msg) { global_log(static_cast<Logger::Level>(0), msg); }
inline void global_log_info(const std::string &msg)  { global_log(static_cast<Logger::Level>(1), msg); }
inline void global_log_warn(const std::string &msg)  { global_log(static_cast<Logger::Level>(2), msg); }
inline void global_log_error(const std::string &msg) { global_log(static_cast<Logger::Level>(3), msg); }