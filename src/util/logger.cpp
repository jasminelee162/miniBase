#include "logger.h"
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <sstream>

Logger::Logger(const std::string &filename)
{
    try {
        std::filesystem::path p(filename);
        if (p.has_parent_path()) {
            std::filesystem::create_directories(p.parent_path());
        }
    } catch (...) {
        // best-effort
    }
    log_file.open(filename, std::ios::app);
}

Logger::~Logger()
{
    std::lock_guard<std::mutex> lk(mu);
    if (log_file.is_open()) log_file.close();
}

const char* Logger::levelToString(Level lvl)
{
    switch (lvl) {
        case Level::DEBUG: return "DEBUG";
        case Level::INFO: return "INFO";
        case Level::WARN: return "WARN";
        case Level::ERROR: return "ERROR";
        default: return "UNK";
    }
}

std::string Logger::timeStamp()
{
    using namespace std::chrono;
    auto now = system_clock::now();
    auto tt = system_clock::to_time_t(now);
    auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
    std::tm bt;
#ifdef _WIN32
    localtime_s(&bt, &tt);
#else
    localtime_r(&tt, &bt);
#endif
    std::ostringstream oss;
    oss << std::put_time(&bt, "%Y-%m-%d %H:%M:%S") << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

void Logger::log(const std::string &msg)
{
    log(Level::INFO, msg);
}

void Logger::log(Level level, const std::string &msg)
{
    if (level < current_level) return;
    std::lock_guard<std::mutex> lk(mu);
    if (!log_file.is_open()) return;
    log_file << "[" << timeStamp() << "] [" << levelToString(level) << "] " << msg << std::endl;
}

void Logger::setLevel(Level level)
{
    std::lock_guard<std::mutex> lk(mu);
    current_level = level;
}

Logger::Level Logger::getLevel() const
{
    return current_level;
}

// ---- Global logger impl ----
static std::unique_ptr<Logger> g_global_logger;

void init_global_logger(const std::string &filename, Logger::Level level)
{
    g_global_logger = std::make_unique<Logger>(filename);
    g_global_logger->setLevel(level);
}

Logger* get_global_logger()
{
    return g_global_logger.get();
}

void global_log(Logger::Level level, const std::string &msg)

{
    if (g_global_logger) g_global_logger->log(level, msg);
}