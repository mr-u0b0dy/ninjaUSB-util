// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2025 Dharun A P

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

namespace args {

/**
 * @brief Program options and configuration
 */
struct Options {
    bool show_help = false;
    bool show_version = false;
    bool verbose = false;
    bool list_devices = false;
    int scan_timeout = 10000;  // Default 10 seconds
    int poll_interval = 1;     // Default 1ms
    std::string target_device; // Specific device to connect to
    std::string log_level = "info";
};

/**
 * @brief Simple command-line argument parser
 */
class ArgumentParser {
private:
    std::string program_name_;
    std::vector<std::string> args_;
    std::unordered_map<std::string, std::string> descriptions_;

public:
    explicit ArgumentParser(int argc, char* argv[]);
    
    /**
     * @brief Parse command line arguments
     * @return Parsed options or nullopt if parsing failed
     */
    [[nodiscard]] std::optional<Options> parse();
    
    /**
     * @brief Show help message
     */
    void show_help() const;
    
    /**
     * @brief Show version information
     */
    void show_version() const;

private:
    void setup_descriptions();
    bool has_flag(const std::string& flag) const;
    std::optional<std::string> get_value(const std::string& flag) const;
    std::optional<int> get_int_value(const std::string& flag) const;
};

} // namespace args
