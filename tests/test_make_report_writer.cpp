/**
 * @file test_make_report_writer.cpp
 * @brief Unit tests for BLE report writer functionality
 * @author Dharun A P
 * @license SPDX-License-Identifier: Apache-2.0
 * @copyright SPDX-FileCopyrightText: 2025 Dharun A P
 */

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

// Mock Qt types for testing
class MockQLowEnergyService {
  public:
    std::vector<uint8_t> last_written_data;
    bool write_called = false;
    bool valid = true;

    void writeCharacteristic(void* /*ch*/, const std::vector<uint8_t>& data, int /*mode*/) {
        write_called = true;
        last_written_data = data;
    }
};

class MockQLowEnergyCharacteristic {
  public:
    bool valid = true;
    bool isValid() const { return valid; }
};

// Simple logging for tests
#define LOG_INFO(msg) std::cout << "[INFO] " << msg << std::endl

// Mock version of make_report_writer for testing
auto make_report_writer_test(MockQLowEnergyService* service, MockQLowEnergyCharacteristic ch) {
    return [service, ch](const std::array<uint8_t, 8>& report) {
        // Validate service and characteristic before writing
        if (!service || !ch.isValid()) {
            LOG_INFO("Invalid service or characteristic, skipping HID report");
            return;
        }

        // Convert HID report to vector and transmit
        std::vector<uint8_t> data(report.begin(), report.end());
        service->writeCharacteristic(nullptr, data, 0);
    };
}

void test_valid_service_and_characteristic() {
    MockQLowEnergyService service;
    MockQLowEnergyCharacteristic ch;

    auto writer = make_report_writer_test(&service, ch);

    std::array<uint8_t, 8> test_report = {0x01, 0x00, 0x04, 0x00,
                                          0x00, 0x00, 0x00, 0x00};  // Ctrl+A
    writer(test_report);

    assert(service.write_called);
    assert(service.last_written_data.size() == 8);
    assert(std::equal(service.last_written_data.begin(), service.last_written_data.end(),
                      test_report.begin()));

    std::cout << "PASSED" << std::endl;
}

void test_null_service() {
    MockQLowEnergyCharacteristic ch;

    auto writer = make_report_writer_test(nullptr, ch);

    std::array<uint8_t, 8> test_report = {0x01, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00};

    // Should not crash when service is null
    writer(test_report);

    std::cout << "PASSED" << std::endl;
}

void test_invalid_characteristic() {
    MockQLowEnergyService service;
    MockQLowEnergyCharacteristic ch;
    ch.valid = false;

    auto writer = make_report_writer_test(&service, ch);

    std::array<uint8_t, 8> test_report = {0x01, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00};
    writer(test_report);

    // Should not call write when characteristic is invalid
    assert(service.write_called == false);

    std::cout << "PASSED" << std::endl;
}

void test_different_reports() {
    MockQLowEnergyService service;
    MockQLowEnergyCharacteristic ch;

    auto writer = make_report_writer_test(&service, ch);

    // Test empty report
    std::array<uint8_t, 8> empty_report = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    writer(empty_report);
    assert(service.write_called);
    assert(service.last_written_data.size() == 8);
    assert(std::equal(service.last_written_data.begin(), service.last_written_data.end(),
                      empty_report.begin()));

    // Reset for next test
    service.write_called = false;
    service.last_written_data.clear();

    // Test modifier-only report (Shift)
    std::array<uint8_t, 8> shift_report = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    writer(shift_report);
    assert(service.write_called);
    assert(std::equal(service.last_written_data.begin(), service.last_written_data.end(),
                      shift_report.begin()));

    // Reset for next test
    service.write_called = false;
    service.last_written_data.clear();

    // Test multiple keys report
    std::array<uint8_t, 8> multi_report = {0x00, 0x00, 0x04, 0x05,
                                           0x06, 0x00, 0x00, 0x00};  // A, B, C
    writer(multi_report);
    assert(service.write_called);
    assert(std::equal(service.last_written_data.begin(), service.last_written_data.end(),
                      multi_report.begin()));

    std::cout << "PASSED" << std::endl;
}

int main() {
    std::cout << "=== Make Report Writer Unit Tests ===" << std::endl;

    try {
        test_valid_service_and_characteristic();
        test_null_service();
        test_invalid_characteristic();
        test_different_reports();

        std::cout << std::endl << "=== All make_report_writer tests completed ===" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
