// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2025 Dharun A P

#include "logger.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>

namespace logging {

// Static member definitions
Level Logger::current_level_ = Level::INFO;
bool Logger::enable_timestamps_ = true;

void Logger::set_level(const std::string& level) {
    std::string lower_level = level;
    std::transform(lower_level.begin(), lower_level.end(), lower_level.begin(), ::tolower);
    
    if (lower_level == "debug") {
        current_level_ = Level::DEBUG;
    } else if (lower_level == "info") {
        current_level_ = Level::INFO;
    } else if (lower_level == "warn" || lower_level == "warning") {
        current_level_ = Level::WARN;
    } else if (lower_level == "error") {
        current_level_ = Level::ERROR;
    }
}

void Logger::debug(const std::string& message) {
    log(Level::DEBUG, message);
}

void Logger::info(const std::string& message) {
    log(Level::INFO, message);
}

void Logger::warn(const std::string& message) {
    log(Level::WARN, message);
}

void Logger::error(const std::string& message) {
    log(Level::ERROR, message);
}

void Logger::log(Level level, const std::string& message) {
    if (level < current_level_) {
        return;
    }
    
    std::ostream& output = (level >= Level::WARN) ? std::cerr : std::cout;
    
    if (enable_timestamps_) {
        output << get_timestamp() << " ";
    }
    
    output << level_to_color(level) << "[" << level_to_string(level) << "] " 
           << message << "\033[0m" << std::endl;
}

std::string Logger::get_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

std::string Logger::level_to_string(Level level) {
    switch (level) {
        case Level::DEBUG: return "DEBUG";
        case Level::INFO:  return "INFO ";
        case Level::WARN:  return "WARN ";
        case Level::ERROR: return "ERROR";
        default:           return "UNKNOWN";
    }
}

std::string Logger::level_to_color(Level level) {
    switch (level) {
        case Level::DEBUG: return "\033[36m";  // Cyan
        case Level::INFO:  return "\033[32m";  // Green
        case Level::WARN:  return "\033[33m";  // Yellow
        case Level::ERROR: return "\033[31m";  // Red
        default:           return "\033[0m";   // Reset
    }
}

} // namespace logging
