#pragma once
#include <atomic>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>

enum class LogLevel { ERROR = 0, WARN = 1, INFO = 2, DEBUG = 3 };

struct LogConfig {
    LogLevel level{LogLevel::INFO};
    std::string role{"?"};
    std::string logFile;
    bool alsoStderr{true};
};

void init_log(const LogConfig &cfg);
void set_log_level(LogLevel lvl);
LogLevel get_log_level();
void log_message(LogLevel lvl, const std::string &role, int connId, const std::string &op,
                 const std::string &msg);

#define LOG_STREAM(msg) ([&](){ std::ostringstream _oss; _oss << msg; return _oss.str(); }())
#define LOG_DEBUG(role, conn, op, msg) \
    do { if (get_log_level() >= LogLevel::DEBUG) log_message(LogLevel::DEBUG, role, conn, op, LOG_STREAM(msg)); } while(0)
#define LOG_INFO(role, conn, op, msg) \
    do { if (get_log_level() >= LogLevel::INFO) log_message(LogLevel::INFO, role, conn, op, LOG_STREAM(msg)); } while(0)
#define LOG_WARN(role, conn, op, msg) \
    do { if (get_log_level() >= LogLevel::WARN) log_message(LogLevel::WARN, role, conn, op, LOG_STREAM(msg)); } while(0)
#define LOG_ERR(role, conn, op, msg) \
    do { if (get_log_level() >= LogLevel::ERROR) log_message(LogLevel::ERROR, role, conn, op, LOG_STREAM(msg)); } while(0)

inline LogLevel level_from_string(const std::string &v) {
    if (v == "debug") return LogLevel::DEBUG;
    if (v == "info") return LogLevel::INFO;
    if (v == "warn") return LogLevel::WARN;
    return LogLevel::ERROR;
}

