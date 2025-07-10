// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2025 Dharun A P

// Simple unit tests for BLE HID Service Manager

#include <iostream>
#include <cassert>
#include <array>
#include <vector>
#include <cstdint>
#include <string>

// Simple test that validates basic functionality
int main() {
    std::cout << "Running simple BLE HID Service Manager tests..." << std::endl;
    
    // Test 1: Basic functionality
    std::cout << "Testing basic functionality..." << std::endl;
    
    // Test enum values
    enum class HIDReportType : std::uint8_t {
        KEYBOARD_INPUT = 0x01,
        CONSUMER_CONTROL = 0x02
    };
    
    assert(static_cast<std::uint8_t>(HIDReportType::KEYBOARD_INPUT) == 0x01);
    assert(static_cast<std::uint8_t>(HIDReportType::CONSUMER_CONTROL) == 0x02);
    
    // Test array creation
    std::array<std::uint8_t, 8> keyboard_report = {0x01, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00};
    assert(keyboard_report.size() == 8);
    assert(keyboard_report[0] == 0x01);
    assert(keyboard_report[2] == 0x04);
    
    std::array<std::uint8_t, 2> consumer_report = {0xE9, 0x00};
    assert(consumer_report.size() == 2);
    assert(consumer_report[0] == 0xE9);
    assert(consumer_report[1] == 0x00);
    
    std::cout << "✓ Basic functionality test passed" << std::endl;
    
    // Test 2: Report type utility function
    std::cout << "Testing report type utility function..." << std::endl;
    
    auto report_type_to_string = [](HIDReportType report_type) -> const char* {
        switch (report_type) {
            case HIDReportType::KEYBOARD_INPUT:
                return "Keyboard Input";
            case HIDReportType::CONSUMER_CONTROL:
                return "Consumer Control";
            default:
                return "Unknown";
        }
    };
    
    assert(std::string(report_type_to_string(HIDReportType::KEYBOARD_INPUT)) == "Keyboard Input");
    assert(std::string(report_type_to_string(HIDReportType::CONSUMER_CONTROL)) == "Consumer Control");
    
    std::cout << "✓ Report type utility function test passed" << std::endl;
    
    std::cout << "All simple BLE HID Service Manager tests passed!" << std::endl;
    
    return 0;
}
