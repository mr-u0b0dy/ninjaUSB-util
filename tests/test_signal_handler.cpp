/**
 * @file test_signal_handler.cpp
 * @brief Unit tests for signal handling functionality
 * @author Dharun A P
 * @license SPDX-License-Identifier: Apache-2.0
 * @copyright SPDX-FileCopyrightText: 2025 Dharun A P
 */

#include <atomic>
#include <cassert>
#include <csignal>
#include <iostream>

// We need to replicate the signal handling logic for testing
// since the actual signal_handler is in main.cpp
std::atomic<bool> g_running_test{true};

void test_signal_handler(int signum) {
    if (signum == SIGINT) {
        // Ignore Ctrl+C to prevent accidental program termination during key capture
        return;
    }

    // For other signals, stop running
    g_running_test = false;
}

namespace {

void test_sigint_ignored() {
    g_running_test = true;
    test_signal_handler(SIGINT);

    // SIGINT should be ignored, so g_running_test should remain true
    assert(g_running_test == true);

    std::cout << "PASSED\n";
}

void test_sigterm_handled() {
    g_running_test = true;
    test_signal_handler(SIGTERM);

    // SIGTERM should set g_running_test to false
    assert(g_running_test == false);

    std::cout << "PASSED\n";
}

void test_other_signals_handled() {
    g_running_test = true;
    test_signal_handler(SIGUSR1);

    // Other signals should set g_running_test to false
    assert(g_running_test == false);

    std::cout << "PASSED\n";
}

}  // namespace

int main() {
    std::cout << "=== Signal Handler Unit Tests ===\n";

    try {
        test_sigint_ignored();
        test_sigterm_handled();
        test_other_signals_handled();

        std::cout << "\n=== All signal handler tests completed ===\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << "\n";
        return 1;
    }
}
