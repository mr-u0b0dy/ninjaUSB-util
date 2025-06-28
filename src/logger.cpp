/**
 * @file logger.cpp
 * @brief Implementation of the logging utility
 * @author Dharun A P
 * @license SPDX-License-Identifier: Apache-2.0
 * @copyright SPDX-FileCopyrightText: 2025 Dharun A P
 */

#include "logger.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>

namespace logging {

/**
 * @brief Static member definitions for Logger class
 * 
 * These static members maintain the global logging state across the application.
 * The default logging level is INFO, and timestamps are enabled by default.
 */
Level Logger::current_level_ = Level::INFO;  //!< Current minimum logging level
bool Logger::enable_timestamps_ = true;      //!< Whether to include timestamps in log output

/**
 * @brief Set the minimum logging level from a string
 * @param level String representation of the logging level (case-insensitive)
 * 
 * Accepts the following level strings:
 * - "debug" -> Level::DEBUG
 * - "info" -> Level::INFO  
 * - "warn" or "warning" -> Level::WARN
 * - "error" -> Level::ERROR
 * 
 * If an unrecognized level is provided, the current level remains unchanged.
 * 
 * @note The string comparison is case-insensitive
 */
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

/**
 * @brief Log a debug message
 * @param message The message to log
 * 
 * Debug messages are only output if the current logging level is DEBUG.
 * These are typically used for detailed diagnostic information during development.
 */
void Logger::debug(const std::string& message) {
    log(Level::DEBUG, message);
}

/**
 * @brief Log an informational message
 * @param message The message to log
 * 
 * Info messages are output if the current logging level is DEBUG or INFO.
 * These are used for general application flow information.
 */
void Logger::info(const std::string& message) {
    log(Level::INFO, message);
}

/**
 * @brief Log a warning message
 * @param message The message to log
 * 
 * Warning messages are output if the current logging level is DEBUG, INFO, or WARN.
 * These indicate potential issues that don't prevent normal operation.
 */
void Logger::warn(const std::string& message) {
    log(Level::WARN, message);
}

/**
 * @brief Log an error message
 * @param message The message to log
 * 
 * Error messages are always output regardless of the current logging level.
 * These indicate serious problems that may affect application functionality.
 */
void Logger::error(const std::string& message) {
    log(Level::ERROR, message);
}

/**
 * @brief Core logging function that handles message formatting and output
 * @param level The severity level of the message
 * @param message The message content to log
 * 
 * This function performs the actual logging work:
 * - Checks if the message level meets the current threshold
 * - Selects appropriate output stream (stdout for info/debug, stderr for warn/error)
 * - Adds timestamp if enabled
 * - Applies color coding based on message level
 * - Outputs the formatted message
 * 
 * @note Messages below the current logging level are silently discarded
 * @note Color codes are ANSI escape sequences for terminal display
 */
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

/**
 * @brief Generate a formatted timestamp string for log messages
 * @return Formatted timestamp string in HH:MM:SS.mmm format
 * 
 * Creates a timestamp with millisecond precision using the system clock.
 * The format is "HH:MM:SS.mmm" where mmm represents milliseconds.
 * 
 * @note Uses local time zone for timestamp display
 * @note Millisecond precision helps with debugging timing-sensitive operations
 */
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

/**
 * @brief Convert logging level enum to human-readable string
 * @param level The logging level to convert
 * @return String representation of the logging level, padded for alignment
 * 
 * Returns fixed-width strings for consistent log formatting:
 * - Level::DEBUG -> "DEBUG"
 * - Level::INFO  -> "INFO "
 * - Level::WARN  -> "WARN "
 * - Level::ERROR -> "ERROR"
 * - Unknown      -> "UNKNOWN"
 * 
 * @note Strings are padded to 5 characters for consistent column alignment
 */
std::string Logger::level_to_string(Level level) {
    switch (level) {
        case Level::DEBUG: return "DEBUG";
        case Level::INFO:  return "INFO ";
        case Level::WARN:  return "WARN ";
        case Level::ERROR: return "ERROR";
        default:           return "UNKNOWN";
    }
}

/**
 * @brief Get ANSI color code for the specified logging level
 * @param level The logging level to get color for
 * @return ANSI escape sequence for terminal color formatting
 * 
 * Returns color codes for visual distinction of log levels:
 * - Level::DEBUG -> Cyan (\033[36m)
 * - Level::INFO  -> Green (\033[32m)
 * - Level::WARN  -> Yellow (\033[33m)
 * - Level::ERROR -> Red (\033[31m)
 * - Unknown      -> Reset (\033[0m)
 * 
 * @note Colors are standard ANSI escape sequences compatible with most terminals
 * @note The reset code (\033[0m) is applied after each message to restore normal colors
 */
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
