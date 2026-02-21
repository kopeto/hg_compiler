#pragma once

#include <iostream>
#include <string>
#include <sstream>

// ANSI color codes
#define LOG_COLOR_RESET  "\033[0m"
#define LOG_COLOR_RED    "\033[31m"
#define LOG_COLOR_YELLOW "\033[33m"
#define LOG_COLOR_CYAN   "\033[36m"
#define LOG_COLOR_GRAY   "\033[90m"

enum class LogLevel {
    NONE  = 0,
    ERROR = 1,
    WARN  = 2,
    INFO  = 3,
    DEBUG = 4,
};

class Logger {
public:
    static LogLevel level;

    static void setLevel(LogLevel l) { level = l; }

    // Base cases (no format args)
    static void error(const std::string& msg) { log(LogLevel::ERROR, msg); }
    static void warn (const std::string& msg) { log(LogLevel::WARN,  msg); }
    static void info (const std::string& msg) { log(LogLevel::INFO,  msg); }
    static void debug(const std::string& msg) { log(LogLevel::DEBUG, msg); }

    // Variadic template versions with {} formatting
    template<typename... Args>
    static void error(const std::string& fmt, Args&&... args) { log(LogLevel::ERROR, format(fmt, std::forward<Args>(args)...)); }

    template<typename... Args>
    static void warn(const std::string& fmt, Args&&... args)  { log(LogLevel::WARN,  format(fmt, std::forward<Args>(args)...)); }

    template<typename... Args>
    static void info(const std::string& fmt, Args&&... args)  { log(LogLevel::INFO,  format(fmt, std::forward<Args>(args)...)); }

    template<typename... Args>
    static void debug(const std::string& fmt, Args&&... args) { log(LogLevel::DEBUG, format(fmt, std::forward<Args>(args)...)); }

private:
    static void log(LogLevel msg_level, const std::string& msg) {
        if (msg_level > level) return;

        const char* color  = LOG_COLOR_RESET;
        const char* prefix = "";

        switch (msg_level) {
            case LogLevel::ERROR: color = LOG_COLOR_RED;    prefix = "ERROR"; break;
            case LogLevel::WARN:  color = LOG_COLOR_YELLOW; prefix = "WARN "; break;
            case LogLevel::INFO:  color = LOG_COLOR_CYAN;   prefix = "INFO "; break;
            case LogLevel::DEBUG: color = LOG_COLOR_GRAY;   prefix = "DEBUG"; break;
            default: break;
        }

        std::cout << color << "[" << prefix << "] " << LOG_COLOR_RESET << msg << "\n";
    }

    // Base case: no more args
    static std::string format(const std::string& fmt) {
        return fmt;
    }

    // Recursive case: replace first {} with next argument
    template<typename T, typename... Args>
    static std::string format(const std::string& fmt, T&& first, Args&&... rest) {
        std::ostringstream oss;
        size_t pos = fmt.find("{}");
        if (pos == std::string::npos) {
            return fmt;
        }
        oss << fmt.substr(0, pos)
            << first
            << format(fmt.substr(pos + 2), std::forward<Args>(rest)...);
        return oss.str();
    }
};

inline LogLevel Logger::level = LogLevel::INFO;