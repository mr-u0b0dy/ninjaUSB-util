/**
 * @file test_logger.cpp
 * @brief Unit tests for logging functionality
 * @author Dharun A P
 * @license SPDX-License-Identifier: Apache-2.0
 * @copyright SPDX-FileCopyrightText: 2025 Dharun A P
 */

#include <cassert>
#include <iostream>
#include <string>

#include "logger.hpp"

namespace {

void test_log_level_filtering() {
    std::cout << "Testing log level filtering... ";

    // Set to INFO level
    logging::Logger::set_level("info");

    // Test that debug messages are filtered out
    // Note: This is a basic test - in a real implementation,
    // we'd need to capture the output to verify filtering

    // These calls should not crash
    LOG_DEBUG("This debug message should be filtered");
    LOG_INFO("This info message should appear");
    LOG_WARN("This warning should appear");
    LOG_ERROR("This error should appear");

    std::cout << "PASSED\n";
}

void test_log_level_changes() {
    std::cout << "Testing log level changes... ";

    // Test various valid log levels
    logging::Logger::set_level("debug");
    logging::Logger::set_level("info");
    logging::Logger::set_level("warn");
    logging::Logger::set_level("error");

    // Test invalid log level (should not crash)
    logging::Logger::set_level("invalid");

    std::cout << "PASSED\n";
}

void test_timestamp_functionality() {
    std::cout << "Testing timestamp functionality... ";

    // Test enabling/disabling timestamps
    logging::Logger::enable_timestamps(true);
    LOG_INFO("Message with timestamp");

    logging::Logger::enable_timestamps(false);
    LOG_INFO("Message without timestamp");

    // Reset to false for other tests
    logging::Logger::enable_timestamps(false);

    std::cout << "PASSED\n";
}

void test_all_log_levels() {
    std::cout << "Testing all log levels... ";

    // Set to debug to see all messages
    logging::Logger::set_level("debug");

    // Test all log level macros
    LOG_DEBUG("Debug message test");
    LOG_INFO("Info message test");
    LOG_WARN("Warning message test");
    LOG_ERROR("Error message test");

    std::cout << "PASSED\n";
}

void test_message_formatting() {
    std::cout << "Testing message formatting... ";

    logging::Logger::set_level("debug");

    // Test various message types
    LOG_INFO("Simple message");
    LOG_INFO("Message with number: " + std::to_string(42));
    LOG_INFO("Message with special chars: @#$%^&*()");
    LOG_INFO("Empty string: " + std::string(""));

    // Test long message
    std::string long_msg(200, 'A');
    LOG_INFO("Long message: " + long_msg);

    std::cout << "PASSED\n";
}

void test_concurrent_logging() {
    std::cout << "Testing concurrent logging safety... ";

    // Basic test - in a real scenario we'd use threads
    // For now, just ensure rapid consecutive calls don't crash
    logging::Logger::set_level("debug");

    for (int i = 0; i < 100; ++i) {
        LOG_DEBUG("Rapid message " + std::to_string(i));
    }

    std::cout << "PASSED\n";
}

void test_level_case_insensitivity() {
    std::cout << "Testing log level case handling... ";

    // Test different cases (behavior depends on implementation)
    logging::Logger::set_level("DEBUG");
    logging::Logger::set_level("Info");
    logging::Logger::set_level("WARN");
    logging::Logger::set_level("error");

    // Should not crash regardless of case
    std::cout << "PASSED\n";
}

}  // namespace

int main() {
    std::cout << "=== Logger Unit Tests ===\n";

    try {
        test_log_level_filtering();
        test_log_level_changes();
        test_timestamp_functionality();
        test_all_log_levels();
        test_message_formatting();
        test_concurrent_logging();
        test_level_case_insensitivity();

        std::cout << "\n=== All logger tests completed ===\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << "\n";
        return 1;
    }
}
