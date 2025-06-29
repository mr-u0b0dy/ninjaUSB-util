/**
 * @file args.cpp
 * @brief Implementation of command-line argument parsing
 * @author Dharun A P
 * @license SPDX-License-Identifier: Apache-2.0
 * @copyright SPDX-FileCopyrightText: 2025 Dharun A P
 */

#include "args.hpp"

#include <algorithm>
#include <iostream>

#include "version.hpp"

namespace args {

/**
 * @brief Construct ArgumentParser from command line arguments
 * @param argc Number of command line arguments
 * @param argv Array of command line argument strings
 *
 * Initializes the argument parser by extracting the program name and storing
 * all arguments for later processing. The program name is cleaned of any
 * directory path components for cleaner help output.
 *
 * @note The first argument (argv[0]) is treated as the program name
 * @note Directory separators (/ and \) are stripped from the program name
 */
ArgumentParser::ArgumentParser(int argc, char* argv[]) {
    if (argc > 0) {
        program_name_ = argv[0];
        // Remove path from program name
        size_t last_slash = program_name_.find_last_of("/\\");
        if (last_slash != std::string::npos) {
            program_name_ = program_name_.substr(last_slash + 1);
        }
    }

    for (int i = 1; i < argc; ++i) {
        args_.emplace_back(argv[i]);
    }

    setup_descriptions();
}

/**
 * @brief Initialize command-line option descriptions for help output
 *
 * Sets up the descriptions map that maps option flags to their explanations.
 * This is used to generate formatted help text that explains each available
 * command-line option and its purpose.
 *
 * @note Descriptions include both short (-X) and long (--xxx) option forms
 * @note Parameter placeholders (like <ms>, <address>) indicate required values
 */
void ArgumentParser::setup_descriptions() {
    descriptions_ = {
        {"-h, --help", "Show this help message and exit"},
        {"-v, --version", "Show version information and exit"},
        {"-V, --verbose", "Enable verbose logging output"},
        {"--list-devices", "List available BLE devices and exit"},
        {"--scan-timeout <ms>", "BLE scan timeout in milliseconds (default: 10000)"},
        {"--poll-interval <ms>", "Input polling interval in milliseconds (default: 1)"},
        {"--target <address>", "Target BLE device address to connect to"},
        {"--log-level <level>", "Set log level (debug, info, warn, error) (default: info)"}};
}

/**
 * @brief Parse command-line arguments and return configuration options
 * @return Optional containing parsed Options, or nullopt if parsing failed
 *
 * Processes all command-line arguments and converts them into a structured
 * Options object. The function handles:
 * - Help and version requests (returns nullopt after displaying info)
 * - Boolean flags (verbose, list-devices)
 * - Value parameters (target address, timeouts, log level)
 * - Input validation and error reporting
 *
 * @retval std::nullopt Parsing failed or help/version was requested
 * @retval Options Parsed configuration ready for application use
 *
 * @note Error messages are printed to stderr before returning nullopt
 * @note Help and version output go to stdout
 */
std::optional<Options> ArgumentParser::parse() {
    Options opts;

    // Check for help and version first
    if (has_flag("-h") || has_flag("--help")) {
        opts.show_help = true;
        return opts;
    }

    if (has_flag("-v") || has_flag("--version")) {
        opts.show_version = true;
        return opts;
    }

    // Parse other options
    opts.verbose = has_flag("-V") || has_flag("--verbose");
    opts.list_devices = has_flag("--list-devices");

    // Parse values with validation
    if (auto timeout = get_int_value("--scan-timeout")) {
        if (*timeout < 1000 || *timeout > 60000) {
            std::cerr << "Error: scan-timeout must be between 1000 and 60000 ms\n";
            return std::nullopt;
        }
        opts.scan_timeout = *timeout;
    }

    if (auto interval = get_int_value("--poll-interval")) {
        if (*interval < 1 || *interval > 1000) {
            std::cerr << "Error: poll-interval must be between 1 and 1000 ms\n";
            return std::nullopt;
        }
        opts.poll_interval = *interval;
    }

    if (auto target = get_value("--target")) {
        opts.target_device = *target;
    }

    if (auto log_level = get_value("--log-level")) {
        const std::vector<std::string> valid_levels = {"debug", "info", "warn", "error"};
        if (std::find(valid_levels.begin(), valid_levels.end(), *log_level) == valid_levels.end()) {
            std::cerr << "Error: invalid log level '" << *log_level << "'\n";
            std::cerr << "Valid levels: debug, info, warn, error\n";
            return std::nullopt;
        }
        opts.log_level = *log_level;
    }

    // Check for unknown arguments
    for (size_t i = 0; i < args_.size(); ++i) {
        const auto& arg = args_[i];

        if (arg.empty() || arg.front() != '-') {
            continue;  // Skip non-option arguments
        }

        // Skip known flags and their values
        if (arg == "-h" || arg == "--help" || arg == "-v" || arg == "--version" || arg == "-V" ||
            arg == "--verbose" || arg == "--list-devices") {
            continue;
        }

        // Skip known options with values
        if (arg == "--scan-timeout" || arg == "--poll-interval" || arg == "--target" ||
            arg == "--log-level") {
            i++;  // Skip the value too
            continue;
        }

        // Check for --option=value format
        bool is_known_option = false;
        if (arg.find('=') != std::string::npos) {
            std::string option_part = arg.substr(0, arg.find('='));
            if (option_part == "--scan-timeout" || option_part == "--poll-interval" ||
                option_part == "--target" || option_part == "--log-level") {
                is_known_option = true;
            }
        }

        if (is_known_option) {
            continue;
        }

        // If we get here, it's an unknown option
        std::cerr << "Error: unknown argument '" << arg << "'\n";
        std::cerr << "Use --help for available options\n";
        return std::nullopt;
    }

    return opts;
}

/**
 * @brief Display comprehensive help information to stdout
 *
 * Shows formatted help text including:
 * - Application name and description
 * - Usage syntax
 * - Detailed option descriptions with proper alignment
 * - Example usage scenarios
 * - Repository link for additional information
 *
 * The output is formatted for readability with consistent indentation
 * and alignment of option descriptions.
 */
void ArgumentParser::show_help() const {
    std::cout << version::APP_NAME << " - " << version::DESCRIPTION << "\n\n";
    std::cout << "USAGE:\n";
    std::cout << "    " << program_name_ << " [OPTIONS]\n\n";
    std::cout << "OPTIONS:\n";

    for (const auto& [flag, desc] : descriptions_) {
        std::cout << "    " << flag;
        // Pad to align descriptions
        if (flag.length() < 25) {
            std::cout << std::string(25 - flag.length(), ' ');
        } else {
            std::cout << "\n" << std::string(29, ' ');
        }
        std::cout << desc << "\n";
    }

    std::cout << "\nEXAMPLES:\n";
    std::cout << "    " << program_name_ << " -V\n";
    std::cout << "        Run with verbose logging\n\n";
    std::cout << "    " << program_name_ << " --list-devices\n";
    std::cout << "        List available BLE devices\n\n";
    std::cout << "    " << program_name_ << " --target AA:BB:CC:DD:EE:FF\n";
    std::cout << "        Connect to specific device\n\n";
    std::cout << "    " << program_name_ << " --scan-timeout 5000 -V\n";
    std::cout << "        Scan for 5 seconds with verbose output\n\n";

    std::cout << "For more information, visit:\n";
    std::cout << version::REPOSITORY << "\n";
}

/**
 * @brief Display version and build information to stdout
 *
 * Shows comprehensive version information including:
 * - Application version details (major.minor.patch-build)
 * - Build timestamp and environment information
 * - Compiler and system details
 *
 * The information is retrieved from the version module which provides
 * both runtime version info and compile-time build details.
 */
void ArgumentParser::show_version() const {
    std::cout << version::get_version_info() << "\n\n";
    std::cout << version::get_build_info() << "\n";
}

/**
 * @brief Check if a specific flag is present in the argument list
 * @param flag The flag to search for (e.g., "-h", "--help")
 * @return true if the flag was found, false otherwise
 *
 * Performs an exact string match against all parsed arguments.
 * This is used for boolean flags that don't take values.
 *
 * @note Case-sensitive comparison
 * @note Only matches exact flag strings, not partial matches
 */
bool ArgumentParser::has_flag(const std::string& flag) const {
    return std::find(args_.begin(), args_.end(), flag) != args_.end();
}

/**
 * @brief Get the value associated with a command-line option
 * @param flag The option flag to search for (e.g., "--target")
 * @return Optional containing the value if found, nullopt otherwise
 *
 * Supports two formats for option values:
 * 1. Separate arguments: "--flag value"
 * 2. Equals format: "--flag=value"
 *
 * The function first searches for the flag as a separate argument and
 * returns the following argument as the value. If not found, it searches
 * for the equals format within any argument.
 *
 * @retval std::nullopt Flag not found or no value provided
 * @retval std::string The value associated with the flag
 *
 * @note Values are returned as-is without validation or conversion
 * @note Empty values (e.g., "--flag=") return empty strings, not nullopt
 */
std::optional<std::string> ArgumentParser::get_value(const std::string& flag) const {
    auto it = std::find(args_.begin(), args_.end(), flag);
    if (it != args_.end() && std::next(it) != args_.end()) {
        return *std::next(it);
    }

    // Check for --flag=value format
    std::string prefix = flag + "=";
    for (const auto& arg : args_) {
        if (arg.length() > prefix.length() && arg.substr(0, prefix.length()) == prefix) {
            return arg.substr(prefix.length());
        }
    }

    return std::nullopt;
}

/**
 * @brief Get an integer value associated with a command-line option
 * @param flag The option flag to search for (e.g., "--scan-timeout")
 * @return Optional containing the parsed integer if valid, nullopt otherwise
 *
 * Retrieves the string value using get_value() and attempts to parse it
 * as an integer using std::stoi(). If parsing fails, an error message
 * is printed to stderr and nullopt is returned.
 *
 * @retval std::nullopt Flag not found, no value provided, or parsing failed
 * @retval int Successfully parsed integer value
 *
 * @note Parsing errors are reported to stderr with the flag name and invalid value
 * @note Accepts standard integer formats supported by std::stoi (decimal, hex, etc.)
 * @note Leading/trailing whitespace is handled by std::stoi
 */
std::optional<int> ArgumentParser::get_int_value(const std::string& flag) const {
    auto value = get_value(flag);
    if (!value)
        return std::nullopt;

    try {
        return std::stoi(*value);
    } catch (const std::exception&) {
        std::cerr << "Error: invalid integer value for " << flag << ": " << *value << "\n";
        return std::nullopt;
    }
}

}  // namespace args
