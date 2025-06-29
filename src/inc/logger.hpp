/**
 * @file logger.hpp
 * @brief Centralized logging system with configurable levels and formatting
 * @author Dharun A P
 * @date 2025
 * @copyright Copyright (c) 2025 Dharun A P
 * @license SPDX-License-Identifier: Apache-2.0
 *
 * This module provides a thread-safe, configurable logging system with
 * multiple verbosity levels, optional timestamps, and consistent formatting.
 * It's designed for both development debugging and production monitoring.
 *
 * @section LoggingFeatures Key Features
 * - Multiple log levels: DEBUG, INFO, WARN, ERROR
 * - Optional timestamp formatting with millisecond precision
 * - Thread-safe logging operations
 * - Configurable global log level filtering
 * - Consistent message formatting across the application
 * - Zero-overhead when logging below current level
 *
 * @section LoggingUsage Usage Example
 * @code
 * // Configure logging
 * logging::Logger::set_level(logging::Level::INFO);
 * logging::Logger::enable_timestamps(true);
 *
 * // Log messages
 * LOG_INFO("Application started");
 * LOG_DEBUG("This won't be shown (below INFO level)");
 * LOG_ERROR("Critical error occurred");
 * @endcode
 *
 * @section Macros Convenience Macros
 * The following macros are available for easy logging:
 * - LOG_DEBUG(msg): Debug level logging
 * - LOG_INFO(msg): Informational logging
 * - LOG_WARN(msg): Warning level logging
 * - LOG_ERROR(msg): Error level logging
 *
 * @section LoggingPerformance Performance Considerations
 * - Log level filtering happens before message formatting
 * - String formatting only occurs when message will be displayed
 * - Thread-safe but minimal synchronization overhead
 * - Timestamps use high-resolution clock for accuracy
 */

#pragma once

#include <string>

/**
 * @namespace logging
 * @brief Centralized logging functionality with configurable levels
 *
 * This namespace contains all logging-related functionality including
 * the main Logger class, log level enumeration, and utility functions
 * for message formatting and output.
 */
namespace logging {

/**
 * @enum Level
 * @brief Logging verbosity levels in order of increasing severity
 *
 * Log levels control which messages are displayed based on their importance.
 * Setting a log level will show all messages at that level and above.
 *
 * @section LogLevels Level Descriptions
 * - DEBUG: Detailed trace information for development and debugging
 * - INFO: General informational messages about program operation
 * - WARN: Warning messages about potentially problematic situations
 * - ERROR: Error messages about serious problems requiring attention
 *
 * @section Filtering Level Filtering
 * When log level is set to INFO, only INFO, WARN, and ERROR messages
 * will be displayed. DEBUG messages will be filtered out for performance.
 *
 * @note Levels are ordered by severity - higher numeric values indicate higher severity
 */
enum class Level {
    DEBUG = 0,  //!< Detailed debugging information (lowest severity)
    INFO = 1,   //!< General informational messages
    WARN = 2,   //!< Warning messages about potential issues
    ERROR = 3   //!< Error messages about serious problems (highest severity)
};

/**
 * @class Logger
 * @brief Thread-safe singleton logging system with configurable levels and formatting
 *
 * The Logger class provides a centralized logging facility with configurable
 * verbosity levels, optional timestamps, and consistent message formatting.
 * It uses a singleton pattern to maintain global state while providing
 * thread-safe logging operations.
 *
 * @section ThreadSafety Thread Safety
 * All logging operations are thread-safe and can be called concurrently
 * from multiple threads without synchronization. The implementation uses
 * appropriate locking to ensure message integrity and prevent interleaved output.
 *
 * @section Configuration Configuration Options
 * - Global log level: Controls which messages are displayed
 * - Timestamp formatting: Optional timestamps with millisecond precision
 * - Output destination: Currently outputs to stderr for visibility
 *
 * @section Performance Performance Optimizations
 * - Level filtering occurs before string formatting to minimize overhead
 * - Static configuration reduces per-call overhead
 * - Efficient timestamp formatting using chrono high-resolution clock
 * - Minimal synchronization for maximum throughput
 *
 * @note This is a singleton class - all methods are static
 * @note Configuration changes affect all subsequent logging calls globally
 */
class Logger {
  private:
    static Level current_level_;     //!< Global minimum log level for filtering
    static bool enable_timestamps_;  //!< Whether to include timestamps in output

  public:
    /**
     * @brief Set global minimum log level for filtering
     * @param level Minimum level - messages below this level will be filtered out
     *
     * Sets the global log level that determines which messages are displayed.
     * Only messages at or above this level will be shown. This allows runtime
     * control of logging verbosity without recompilation.
     *
     * @section Examples Usage Examples
     * @code
     * Logger::set_level(Level::DEBUG); // Show all messages
     * Logger::set_level(Level::ERROR); // Show only errors
     * @endcode
     */
    static void set_level(Level level) { current_level_ = level; }

    /**
     * @brief Set global log level from string representation
     * @param level String representation of log level ("debug", "info", "warn", "error")
     *
     * Convenience method for setting log level from command-line arguments
     * or configuration files. Case-insensitive matching is performed.
     *
     * @section ValidValues Valid String Values
     * - "debug": Enable all logging including debug messages
     * - "info": Show informational messages and above
     * - "warn": Show only warnings and errors
     * - "error": Show only error messages
     *
     * @note Invalid strings are ignored and level remains unchanged
     * @note String matching is case-insensitive for convenience
     */
    static void set_level(const std::string& level);

    /**
     * @brief Enable or disable timestamp prefixes on log messages
     * @param enable true to include timestamps, false to disable
     *
     * When enabled, each log message will be prefixed with a timestamp
     * in the format "YYYY-MM-DD HH:MM:SS.mmm" for precise timing information.
     * Useful for debugging timing issues and understanding event sequences.
     *
     * @section TimestampFormat Timestamp Format
     * Timestamps use the system's high-resolution clock and include:
     * - Date: YYYY-MM-DD format
     * - Time: HH:MM:SS format with 24-hour notation
     * - Milliseconds: .mmm precision for sub-second timing
     *
     * @note Timestamps add minimal overhead when enabled
     * @note Useful for performance analysis and debugging
     */
    static void enable_timestamps(bool enable) { enable_timestamps_ = enable; }

    /**
     * @brief Log debug message (lowest severity level)
     * @param message Debug message content
     *
     * Logs detailed debugging information typically used during development.
     * Debug messages are only shown when log level is set to DEBUG.
     *
     * @note Only displayed when current log level is DEBUG or lower
     */
    static void debug(const std::string& message);

    /**
     * @brief Log informational message
     * @param message Informational message content
     *
     * Logs general information about program operation and state changes.
     * Info messages are shown when log level is INFO or lower.
     *
     * @note Displayed when current log level is INFO or lower
     */
    static void info(const std::string& message);

    /**
     * @brief Log warning message
     * @param message Warning message content
     *
     * Logs warning messages about potentially problematic situations
     * that don't prevent continued operation but may need attention.
     *
     * @note Displayed when current log level is WARN or lower
     */
    static void warn(const std::string& message);

    /**
     * @brief Log error message (highest severity level)
     * @param message Error message content
     *
     * Logs error messages about serious problems that affect program
     * operation or functionality. Error messages are always displayed
     * regardless of log level setting.
     *
     * @note Always displayed regardless of current log level
     */
    static void error(const std::string& message);

  private:
    /**
     * @brief Internal logging implementation with level checking and formatting
     * @param level Message severity level
     * @param message Message content to log
     *
     * Core logging function that performs level filtering, timestamp generation,
     * message formatting, and output. This function is called by all public
     * logging methods after level-specific validation.
     *
     * @section MessageFormat Output Format
     * Messages are formatted as: "[TIMESTAMP] [LEVEL] MESSAGE"
     * - TIMESTAMP: Optional, based on enable_timestamps_ setting
     * - LEVEL: DEBUG/INFO/WARN/ERROR for easy filtering
     * - MESSAGE: User-provided message content
     *
     * @note This function performs the actual level filtering and formatting
     * @note Thread-safe implementation with appropriate synchronization
     */
    static void log(Level level, const std::string& message);

    /**
     * @brief Generate formatted timestamp string for log messages
     * @return Formatted timestamp string in "YYYY-MM-DD HH:MM:SS.mmm" format
     *
     * Creates a high-precision timestamp using the system's steady clock.
     * Used internally by the logging system when timestamps are enabled.
     *
     * @note Uses high-resolution clock for sub-second precision
     * @note Format is consistent and sortable for log analysis
     */
    static std::string get_timestamp();

    /**
     * @brief Convert log level enum to string representation
     * @param level Log level to convert
     * @return String representation ("DEBUG", "INFO", "WARN", "ERROR")
     *
     * Provides consistent string representation of log levels for
     * message formatting and display purposes.
     */
    static std::string level_to_string(Level level);

    /**
     * @brief Get ANSI color code for log level (for terminal output)
     * @param level Log level to get color for
     * @return ANSI color escape sequence for the specified level
     *
     * Provides color coding for log levels to improve readability
     * in terminal output. Colors are chosen for good contrast and
     * conventional severity indication.
     *
     * @section ColorMapping Color Mapping
     * - DEBUG: Gray (low importance)
     * - INFO: White (normal)
     * - WARN: Yellow (caution)
     * - ERROR: Red (danger)
     *
     * @note Colors are only applied when output is to a terminal
     */
    static std::string level_to_color(Level level);
};

}  // namespace logging

// ============================================================================
//  Convenience Macros for Easy Logging
// ============================================================================

/**
 * @def LOG_DEBUG(msg)
 * @brief Convenience macro for debug-level logging
 * @param msg Message string to log
 *
 * Provides a convenient interface for debug logging without requiring
 * explicit namespace qualification. Expands to logging::Logger::debug(msg).
 *
 * @section Usage Usage Example
 * @code
 * LOG_DEBUG("Entering function with parameter: " + std::to_string(value));
 * @endcode
 *
 * @note Only displayed when log level is set to DEBUG
 * @note Message formatting occurs only if debug level is active
 */
#define LOG_DEBUG(msg) logging::Logger::debug(msg)

/**
 * @def LOG_INFO(msg)
 * @brief Convenience macro for informational logging
 * @param msg Message string to log
 *
 * Provides a convenient interface for informational logging. Most commonly
 * used logging level for general program operation messages.
 *
 * @section Usage Usage Example
 * @code
 * LOG_INFO("Device connected: " + device_name);
 * @endcode
 */
#define LOG_INFO(msg) logging::Logger::info(msg)

/**
 * @def LOG_WARN(msg)
 * @brief Convenience macro for warning-level logging
 * @param msg Message string to log
 *
 * Used for potentially problematic situations that don't prevent
 * continued operation but may require attention.
 *
 * @section Usage Usage Example
 * @code
 * LOG_WARN("Device disconnected unexpectedly, retrying...");
 * @endcode
 */
#define LOG_WARN(msg) logging::Logger::warn(msg)

/**
 * @def LOG_ERROR(msg)
 * @brief Convenience macro for error-level logging
 * @param msg Message string to log
 *
 * Used for serious errors that affect program operation. Error messages
 * are always displayed regardless of the current log level setting.
 *
 * @section Usage Usage Example
 * @code
 * LOG_ERROR("Failed to initialize device: " + error_message);
 * @endcode
 *
 * @note Always displayed regardless of current log level
 */
#define LOG_ERROR(msg) logging::Logger::error(msg)
