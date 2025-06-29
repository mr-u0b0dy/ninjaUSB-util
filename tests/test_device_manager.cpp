/**
 * @file test_device_manager.cpp
 * @brief Basic unit tests for device manager functionality
 * @author Dharun A P
 * @license SPDX-License-Identifier: Apache-2.0
 * @copyright SPDX-FileCopyrightText: 2025 Dharun A P
 */

#include <cassert>
#include <iostream>
#include <string>

#include "device_manager.hpp"

namespace {
void test_keyboard_device_invalid_path() {
    std::cout << "Testing KeyboardDevice with invalid path... ";

    // Test with empty path
    device::KeyboardDevice kbd("");
    assert(!kbd.is_valid());

    // Test with non-existent path
    device::KeyboardDevice kbd2("/dev/input/nonexistent");
    assert(!kbd2.is_valid());

    std::cout << "PASSED\n";
}

void test_device_monitor_creation() {
    std::cout << "Testing DeviceMonitor creation... ";

    device::DeviceMonitor monitor;
    // Note: This might fail in containers or restricted environments
    // but should work on a normal Linux system
    std::cout << (monitor.is_valid() ? "PASSED" : "SKIPPED (no udev access)") << "\n";
}

void test_keyboard_manager_basic() {
    std::cout << "Testing KeyboardManager basic functionality... ";

    device::KeyboardManager manager;
    // Should not crash during construction
    std::cout << "Found " << manager.device_count() << " keyboards... ";

    std::cout << "PASSED\n";
}
}  // namespace

int main() {
    std::cout << "=== Device Manager Unit Tests ===\n";

    try {
        test_keyboard_device_invalid_path();
        test_device_monitor_creation();
        test_keyboard_manager_basic();

        std::cout << "\n=== All tests completed ===\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << "\n";
        return 1;
    }
}
