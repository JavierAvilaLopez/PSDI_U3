#include "logging.h"
#include <chrono>
#include <iostream>
#include <thread>
#include <unistd.h>

namespace {
std::mutex log_mutex;
LogConfig current_cfg{};
}

void init_log(const LogConfig &cfg) {
    std::lock_guard<std::mutex> lk(log_mutex);
    current_cfg = cfg;
    if (!current_cfg.logFile.empty()) {
        std::ofstream f(current_cfg.logFile, std::ios::app);
        if (!f) {
            std::cerr << "[logging] cannot open log file: " << current_cfg.logFile << std::endl;
            current_cfg.logFile.clear();
        }
    }
}

void set_log_level(LogLevel lvl) {
    std::lock_guard<std::mutex> lk(log_mutex);
    current_cfg.level = lvl;
}

LogLevel get_log_level() {
    std::lock_guard<std::mutex> lk(log_mutex);
    return current_cfg.level;
}

static std::string timestamp() {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto t = system_clock::to_time_t(now);
    auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
    std::tm tm{};
    gmtime_r(&t, &tm);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S") << '.' << std::setw(3) << std::setfill('0') << ms.count() << 'Z';
    return oss.str();
}

void log_message(LogLevel lvl, const std::string &role, int connId, const std::string &op,
                 const std::string &msg) {
    std::lock_guard<std::mutex> lk(log_mutex);
    if (lvl > current_cfg.level) return;
    std::ostringstream oss;
    oss << '[' << timestamp() << ']'
        << "[pid=" << getpid() << "]"
        << "[tid=" << std::this_thread::get_id() << "]"
        << "[" << role << "]"
        << "[conn=" << connId << "]"
        << "[op=" << op << "]";
    switch (lvl) {
        case LogLevel::DEBUG: oss << "[DEBUG] "; break;
        case LogLevel::INFO: oss << "[INFO ] "; break;
        case LogLevel::WARN: oss << "[WARN ] "; break;
        default: oss << "[ERROR] "; break;
    }
    oss << msg << '\n';
    std::string line = oss.str();
    if (current_cfg.alsoStderr) std::cerr << line;
    if (!current_cfg.logFile.empty()) {
        std::ofstream f(current_cfg.logFile, std::ios::app);
        if (f) f << line;
    }
}

