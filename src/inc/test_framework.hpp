/**
 * @file test_framework.hpp
 * @brief Common test framework utilities to eliminate code duplication
 * @author Dharun A P
 * @license SPDX-License-Identifier: Apache-2.0
 * @copyright SPDX-FileCopyrightText: 2025 Dharun A P
 */

#pragma once

#include <exception>
#include <functional>
#include <iostream>
#include <vector>

namespace test_framework {

/**
 * @brief Structure to hold test information
 */
struct TestCase {
    std::string name;
    std::function<void()> test_function;

    TestCase(const std::string& test_name, std::function<void()> test_func)
        : name(test_name), test_function(test_func) {}
};

/**
 * @brief Wrapper function to add standard test output
 * @param test_name Name of the test for output
 * @param test_func Test function to execute
 */
inline void run_test_with_output(const std::string& test_name, std::function<void()> test_func) {
    std::cout << "Testing " << test_name << "... ";
    test_func();
    std::cout << "PASSED\n";
}

/**
 * @brief Run a series of test cases with common error handling
 * @param test_suite_name Name of the test suite for display
 * @param test_cases Vector of test cases to execute
 * @return 0 on success, 1 on failure
 */
inline int run_test_suite(const std::string& test_suite_name,
                          const std::vector<TestCase>& test_cases) {
    std::cout << "=== " << test_suite_name << " ===\n";

    try {
        for (const auto& test_case : test_cases) {
            run_test_with_output(test_case.name, test_case.test_function);
        }

        std::cout << "\n=== All " << test_suite_name.substr(0, test_suite_name.find(" "))
                  << " tests completed ===\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << "\n";
        return 1;
    }
}

}  // namespace test_framework
