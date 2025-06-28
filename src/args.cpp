// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2025 Dharun A P

#include "args.hpp"
#include "version.hpp"
#include <iostream>
#include <algorithm>

namespace args {

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

void ArgumentParser::setup_descriptions() {
    descriptions_ = {
        {"-h, --help", "Show this help message and exit"},
        {"-v, --version", "Show version information and exit"},
        {"-V, --verbose", "Enable verbose logging output"},
        {"--list-devices", "List available BLE devices and exit"},
        {"--scan-timeout <ms>", "BLE scan timeout in milliseconds (default: 10000)"},
        {"--poll-interval <ms>", "Input polling interval in milliseconds (default: 1)"},
        {"--target <address>", "Target BLE device address to connect to"},
        {"--log-level <level>", "Set log level (debug, info, warn, error) (default: info)"}
    };
}

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
            continue; // Skip non-option arguments
        }
        
        // Skip known flags and their values
        if (arg == "-h" || arg == "--help" || 
            arg == "-v" || arg == "--version" ||
            arg == "-V" || arg == "--verbose" || arg == "--list-devices") {
            continue;
        }
        
        // Skip known options with values
        if (arg == "--scan-timeout" || arg == "--poll-interval" ||
            arg == "--target" || arg == "--log-level") {
            i++; // Skip the value too
            continue;
        }
        
        // Check for --option=value format  
        bool is_known_option = false;
        if (arg.find('=') != std::string::npos) {
            std::string option_part = arg.substr(0, arg.find('='));
            if (option_part == "--scan-timeout" ||
                option_part == "--poll-interval" ||
                option_part == "--target" ||
                option_part == "--log-level") {
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

void ArgumentParser::show_version() const {
    std::cout << version::get_version_info() << "\n\n";
    std::cout << version::get_build_info() << "\n";
}

bool ArgumentParser::has_flag(const std::string& flag) const {
    return std::find(args_.begin(), args_.end(), flag) != args_.end();
}

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

std::optional<int> ArgumentParser::get_int_value(const std::string& flag) const {
    auto value = get_value(flag);
    if (!value) return std::nullopt;
    
    try {
        return std::stoi(*value);
    } catch (const std::exception&) {
        std::cerr << "Error: invalid integer value for " << flag << ": " << *value << "\n";
        return std::nullopt;
    }
}

} // namespace args
