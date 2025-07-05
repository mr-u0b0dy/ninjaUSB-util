/**
 * @file test_args.cpp
 * @brief Unit tests for argument parsing functionality
 * @author Dharun A P
 * @license SPDX-License-Identifier: Apache-2.0
 * @copyright SPDX-FileCopyrightText: 2025 Dharun A P
 */

#include <cassert>
#include <ios>
#include <vector>

#include "args.hpp"
#include "test_framework.hpp"

namespace {
// Helper to create argc/argv from vector of strings
std::pair<int, char**> make_argv(const std::vector<std::string>& args) {
    static std::vector<std::string> stored_args;  // Store actual strings
    static std::vector<char*> argv_ptrs;

    stored_args = args;  // Copy the strings to ensure they persist
    argv_ptrs.clear();
    argv_ptrs.reserve(stored_args.size());

    for (auto& arg : stored_args) {
        argv_ptrs.push_back(const_cast<char*>(arg.c_str()));
    }

    return {static_cast<int>(argv_ptrs.size()), argv_ptrs.data()};
}

void test_help_option() {
    auto [argc, argv] = make_argv({"ninja_util", "--help"});
    args::ArgumentParser parser(argc, argv);
    auto opts = parser.parse();

    assert(opts.has_value());
    assert(opts->show_help == true);
    assert(opts->show_version == false);

    std::cout << "PASSED\n";
}

void test_version_option() {
    auto [argc, argv] = make_argv({"ninja_util", "-v"});
    args::ArgumentParser parser(argc, argv);
    auto opts = parser.parse();

    assert(opts.has_value());
    assert(opts->show_version == true);
    assert(opts->show_help == false);

    std::cout << "PASSED\n";
}

void test_verbose_option() {
    // Test --verbose
    auto [argc1, argv1] = make_argv({"ninja_util", "--verbose"});
    args::ArgumentParser parser1(argc1, argv1);
    auto opts1 = parser1.parse();

    assert(opts1.has_value());
    assert(opts1->verbose == true);

    // Test -V
    auto [argc2, argv2] = make_argv({"ninja_util", "-V"});
    args::ArgumentParser parser2(argc2, argv2);
    auto opts2 = parser2.parse();

    assert(opts2.has_value());
    assert(opts2->verbose == true);

    std::cout << "PASSED\n";
}

void test_scan_timeout_option() {
    auto [argc, argv] = make_argv({"ninja_util", "--scan-timeout", "5000"});
    args::ArgumentParser parser(argc, argv);
    auto opts = parser.parse();

    assert(opts.has_value());
    assert(opts->scan_timeout == 5000);

    std::cout << "PASSED\n";
}

void test_invalid_option() {
    // Test unknown flag
    auto [argc1, argv1] = make_argv({"ninja_util", "--unknown-flag"});
    args::ArgumentParser parser1(argc1, argv1);

    // Capture stderr temporarily to avoid cluttering test output
    std::cerr.setstate(std::ios_base::failbit);
    auto opts1 = parser1.parse();
    std::cerr.clear();

    // Should return nullopt for unknown options
    assert(!opts1.has_value());

    // Test invalid timeout value (out of range)
    auto [argc2, argv2] = make_argv({"ninja_util", "--scan-timeout", "100000"});
    args::ArgumentParser parser2(argc2, argv2);

    std::cerr.setstate(std::ios_base::failbit);
    auto opts2 = parser2.parse();
    std::cerr.clear();

    // Should return nullopt for out-of-range values
    assert(!opts2.has_value());

    // Test invalid log level
    auto [argc3, argv3] = make_argv({"ninja_util", "--log-level", "invalid"});
    args::ArgumentParser parser3(argc3, argv3);

    std::cerr.setstate(std::ios_base::failbit);
    auto opts3 = parser3.parse();
    std::cerr.clear();

    // Should return nullopt for invalid log levels
    assert(!opts3.has_value());

    std::cout << "PASSED\n";
}

void test_log_level_option() {
    auto [argc, argv] = make_argv({"ninja_util", "--log-level", "debug"});
    args::ArgumentParser parser(argc, argv);
    auto opts = parser.parse();

    assert(opts.has_value());
    assert(opts->log_level == "debug");

    std::cout << "PASSED\n";
}

void test_combined_options() {
    auto [argc, argv] =
        make_argv({"ninja_util", "-V", "--scan-timeout", "3000", "--log-level", "warn"});
    args::ArgumentParser parser(argc, argv);
    auto opts = parser.parse();

    assert(opts.has_value());
    assert(opts->verbose == true);
    assert(opts->scan_timeout == 3000);
    assert(opts->log_level == "warn");

    std::cout << "PASSED\n";
}

void test_disable_auto_connect_option() {
    auto [argc, argv] = make_argv({"ninja_util", "--disable-auto-connect"});
    args::ArgumentParser parser(argc, argv);
    auto opts = parser.parse();

    assert(opts.has_value());
    assert(opts->disable_auto_connect == true);

    // Test default value
    auto [argc2, argv2] = make_argv({"ninja_util"});
    args::ArgumentParser parser2(argc2, argv2);
    auto opts2 = parser2.parse();

    assert(opts2.has_value());
    assert(opts2->disable_auto_connect == false);

    std::cout << "PASSED\n";
}

void test_list_devices_option() {
    auto [argc, argv] = make_argv({"ninja_util", "--list-devices"});
    args::ArgumentParser parser(argc, argv);
    auto opts = parser.parse();

    assert(opts.has_value());
    assert(opts->list_devices == true);

    std::cout << "PASSED\n";
}

void test_target_device_option() {
    auto [argc, argv] = make_argv({"ninja_util", "--target", "AA:BB:CC:DD:EE:FF"});
    args::ArgumentParser parser(argc, argv);
    auto opts = parser.parse();

    assert(opts.has_value());
    assert(opts->target_device == "AA:BB:CC:DD:EE:FF");

    std::cout << "PASSED\n";
}

void test_poll_interval_option() {
    auto [argc, argv] = make_argv({"ninja_util", "--poll-interval", "5"});
    args::ArgumentParser parser(argc, argv);
    auto opts = parser.parse();

    assert(opts.has_value());
    assert(opts->poll_interval == 5);

    std::cout << "PASSED\n";
}
}  // namespace

int main() {
    return test_framework::run_test_suite(
        "Argument Parser Unit Tests",
        {{"help option", test_help_option},
         {"version option", test_version_option},
         {"verbose option", test_verbose_option},
         {"scan timeout option", test_scan_timeout_option},
         {"invalid option handling", test_invalid_option},
         {"log level option", test_log_level_option},
         {"combined options", test_combined_options},
         {"disable auto connect option", test_disable_auto_connect_option},
         {"list devices option", test_list_devices_option},
         {"target device option", test_target_device_option},
         {"poll interval option", test_poll_interval_option}});
}
