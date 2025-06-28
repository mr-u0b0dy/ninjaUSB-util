// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2025 Dharun A P

#pragma once

#include <iostream>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace logging {

enum class Level {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3
};

/**
 * @brief Simple logging utility with levels and timestamps
 */
class Logger {
private:
    static Level current_level_;
    static bool enable_timestamps_;

public:
    /**
     * @brief Set global log level
     */
    static void set_level(Level level) { current_level_ = level; }
    static void set_level(const std::string& level);
    
    /**
     * @brief Enable/disable timestamps
     */
    static void enable_timestamps(bool enable) { enable_timestamps_ = enable; }
    
    /**
     * @brief Log messages at different levels
     */
    static void debug(const std::string& message);
    static void info(const std::string& message);
    static void warn(const std::string& message);
    static void error(const std::string& message);

private:
    static void log(Level level, const std::string& message);
    static std::string get_timestamp();
    static std::string level_to_string(Level level);
    static std::string level_to_color(Level level);
};

// Convenience macros for logging
#define LOG_DEBUG(msg) logging::Logger::debug(msg)
#define LOG_INFO(msg) logging::Logger::info(msg)
#define LOG_WARN(msg) logging::Logger::warn(msg)
#define LOG_ERROR(msg) logging::Logger::error(msg)

} // namespace logging
