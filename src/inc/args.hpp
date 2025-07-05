/**
 * @file args.hpp
 * @brief Command-line argument parsing and program configuration
 * @author Dharun A P
 * @date 2025
 * @copyright Copyright (c) 2025 Dharun A P
 * @license SPDX-License-Identifier: Apache-2.0
 *
 * This module provides comprehensive command-line argument parsing with
 * validation, help generation, and configuration management. It supports
 * both short and long options with flexible parameter handling.
 *
 * @section SupportedOptions Supported Options
 * - `--help, -h`: Display usage information and exit
 * - `--version, -v`: Show version and build information
 * - `--verbose, -V`: Enable verbose logging with timestamps
 * - `--list-devices, -l`: List available BLE devices and exit
 * - `--target <address>`: Connect to specific BLE device by MAC address
 * - `--scan-timeout <ms>`: Set BLE device scanning timeout in milliseconds
 * - `--poll-interval <ms>`: Set input polling interval in milliseconds
 * - `--log-level <level>`: Set logging verbosity (debug, info, error)
 *
 * @section ArgumentErrorHandling Error Handling
 * - Invalid options: Show error message and usage information
 * - Missing required values: Clear error reporting with examples
 * - Type validation: Automatic conversion with range checking
 * - Graceful failures: Never throws exceptions, uses optional returns
 *
 * @section UsageExample Usage Example
 * @code
 * args::ArgumentParser parser(argc, argv);
 * auto options = parser.parse();
 * if (!options) {
 *     return 1; // Parse error already reported
 * }
 *
 * if (options->show_help) {
 *     parser.show_help();
 *     return 0;
 * }
 *
 * // Use parsed options...
 * @endcode
 */

#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * @namespace args
 * @brief Command-line argument parsing and configuration management
 *
 * This namespace contains utilities for parsing command-line arguments,
 * validating options, and managing program configuration. It provides
 * a type-safe, exception-free interface for argument processing.
 */
namespace args {

/**
 * @struct Options
 * @brief Program configuration options parsed from command-line arguments
 *
 * This structure contains all configuration options that can be specified
 * via command-line arguments. It provides sensible defaults and validates
 * ranges where appropriate.
 *
 * @section DefaultValues Default Values
 * - scan_timeout: 10000ms (10 seconds) - reasonable time for device discovery
 * - poll_interval: 1ms - responsive input processing without excessive CPU usage
 * - log_level: "info" - balanced verbosity for normal operation
 * - All boolean flags: false - opt-in behavior
 *
 * @section Validation Value Validation
 * - Timeouts: Must be positive values
 * - Log levels: Must be one of "debug", "info", "error"
 * - BLE addresses: Must be valid MAC address format (AA:BB:CC:DD:EE:FF)
 * - Intervals: Must be >= 1ms to prevent excessive CPU usage
 */
struct Options {
    bool show_help = false;     //!< Display help message and exit
    bool show_version = false;  //!< Display version information and exit
    bool verbose = false;       //!< Enable verbose logging with timestamps
    bool list_devices = false;  //!< List available BLE devices and exit
    bool disable_auto_connect = false;  //!< Disable automatic connection to single NinjaUSB device
    int scan_timeout = 10000;   //!< BLE device scanning timeout in milliseconds (default: 10s)
    int poll_interval = 1;      //!< Input device polling interval in milliseconds (default: 1ms)
    std::string target_device;  //!< Specific BLE device MAC address to connect to (optional)
    std::string log_level = "info";  //!< Logging verbosity level (debug, info, error)
};

/**
 * @class ArgumentParser
 * @brief Robust command-line argument parser with validation and help generation
 *
 * This class provides comprehensive argument parsing with support for both
 * short and long options, automatic help generation, and extensive validation.
 * It follows Unix conventions for argument handling and provides clear error reporting.
 *
 * @section Features Key Features
 * - Support for both short (-v) and long (--verbose) options
 * - Automatic help and usage generation
 * - Type-safe option parsing with validation
 * - Clear error messages with usage hints
 * - Exception-free design using std::optional
 * - Extensible design for adding new options
 *
 * @section ArgumentFormats Supported Argument Formats
 * - Boolean flags: `-v`, `--verbose`
 * - Options with values: `--target AA:BB:CC:DD:EE:FF`
 * - Space-separated values: `--scan-timeout 5000`
 * - Equals-separated values: `--log-level=debug`
 * - Short option combining: `-Vl` (equivalent to `-V -l`)
 *
 * @section ErrorHandling Error Handling
 * - Unknown options: Reports unrecognized option and shows usage
 * - Missing values: Shows expected format and examples
 * - Invalid values: Validates ranges and formats with helpful messages
 * - Parse errors: Returns nullopt instead of throwing exceptions
 *
 * @note The parser is designed to be user-friendly with clear error messages
 * @note All methods are const-correct and thread-safe after construction
 */
class ArgumentParser {
  private:
    std::string program_name_;       //!< Program name extracted from argv[0]
    std::vector<std::string> args_;  //!< Command-line arguments as strings
    std::unordered_map<std::string, std::string> descriptions_;  //!< Option descriptions for help

  public:
    /**
     * @brief Construct parser from command-line arguments
     * @param argc Number of command-line arguments
     * @param argv Array of command-line argument strings
     *
     * Initializes the parser with command-line arguments and extracts
     * the program name for use in help and error messages. The program
     * name is cleaned to show only the basename without directory path.
     */
    explicit ArgumentParser(int argc, char* argv[]);

    /**
     * @brief Parse command-line arguments into Options structure
     * @return Parsed options on success, nullopt on parse error
     *
     * Processes all command-line arguments and validates them against
     * the supported option set. Returns a fully populated Options
     * structure on success, or nullopt if any errors occur.
     *
     * @section ParseProcess Parse Process
     * 1. Iterate through all arguments
     * 2. Identify option type (short/long, boolean/value)
     * 3. Validate option names and required values
     * 4. Convert and validate option values
     * 5. Populate Options structure with validated values
     *
     * @section ErrorReporting Error Reporting
     * Parse errors are reported to stderr with:
     * - Clear description of the problem
     * - The specific argument that caused the error
     * - Suggestion for correct usage
     * - Reference to --help for full usage information
     *
     * @note This method does not throw exceptions - errors are reported via stderr
     * @note Options validation includes range checking and format validation
     */
    [[nodiscard]] std::optional<Options> parse();

    /**
     * @brief Display comprehensive help message to stdout
     *
     * Shows detailed usage information including:
     * - Program description and purpose
     * - Complete option list with descriptions
     * - Usage examples for common scenarios
     * - Version and license information
     *
     * The help message is formatted for readability with proper alignment
     * and includes both short and long option forms where applicable.
     */
    void show_help() const;

    /**
     * @brief Display version and build information to stdout
     *
     * Shows comprehensive version information including:
     * - Application name and version number
     * - Build date and time
     * - Git commit hash (if available)
     * - Compiler and Qt version information
     * - License and copyright notice
     *
     * This information is useful for debugging and support purposes.
     */
    void show_version() const;

  private:
    void setup_descriptions();
    bool has_flag(const std::string& flag) const;
    std::optional<std::string> get_value(const std::string& flag) const;
    std::optional<int> get_int_value(const std::string& flag) const;
};

}  // namespace args
