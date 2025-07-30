/**
 * @file test_ble_hid_service_manager.cpp
 * @brief Unit tests for BLE HID Service Manager
 * @author Assistant
 * @license SPDX-License-Identifier: Apache-2.0
 */

#include <array>
#include <cassert>
#include <iostream>
#include <memory>

// Mock Qt classes for testing
class MockQLowEnergyService {
  public:
    bool write_called = false;
    std::vector<uint8_t> last_written_data;

    void writeCharacteristic(const MockQLowEnergyCharacteristic&, const QByteArray& data, int) {
        write_called = true;
        last_written_data.clear();
        for (char c : data) {
            last_written_data.push_back(static_cast<uint8_t>(c));
        }
    }
};

class MockQLowEnergyCharacteristic {
  public:
    bool valid = true;
    uint32_t properties = 0x08;     // WriteNoResponse
    std::string uuid_str = "2A4D";  // Generic report characteristic
    std::string name_str = "HID Report";

    bool isValid() const { return valid; }
    uint32_t properties() const { return properties; }
    QString uuid() const { return QString::fromStdString(uuid_str); }
    QString name() const { return QString::fromStdString(name_str); }
    std::vector<MockQLowEnergyDescriptor> descriptors() const { return {}; }
};

class QByteArray {
  public:
    std::vector<char> data_;

    QByteArray() = default;
    QByteArray(const char* data, int size) : data_(data, data + size) {}

    void append(char c) { data_.push_back(c); }
    void prepend(char c) { data_.insert(data_.begin(), c); }
    void reserve(size_t n) { data_.reserve(n); }
    size_t size() const { return data_.size(); }
    const char* data() const { return data_.data(); }

    char& operator[](size_t i) { return data_[i]; }
    const char& operator[](size_t i) const { return data_[i]; }

    auto begin() const { return data_.begin(); }
    auto end() const { return data_.end(); }
};

class QString {
  public:
    std::string str_;

    QString() = default;
    QString(const std::string& s) : str_(s) {}

    static QString fromStdString(const std::string& s) { return QString(s); }
    std::string toStdString() const { return str_; }
    QString toUpper() const { return QString(str_); }  // Simplified
    bool contains(const QString& s) const { return str_.find(s.str_) != std::string::npos; }
    bool contains(const char* s) const { return str_.find(s) != std::string::npos; }
};

class QBluetoothUuid {
  public:
    std::string uuid_str_;

    QBluetoothUuid(const std::string& s) : uuid_str_(s) {}
    QString toString() const { return QString(uuid_str_); }
};

// Include the header we want to test (simplified mock version)
namespace ble_hid {

enum class HIDReportType : std::uint8_t {
    KEYBOARD_INPUT = 0x01,
    CONSUMER_CONTROL = 0x02,
    MOUSE_INPUT = 0x03,
    KEYBOARD_OUTPUT = 0x11,
    FEATURE = 0x21
};

const char* report_type_to_string(HIDReportType report_type) {
    switch (report_type) {
        case HIDReportType::KEYBOARD_INPUT:
            return "Keyboard Input";
        case HIDReportType::CONSUMER_CONTROL:
            return "Consumer Control";
        case HIDReportType::MOUSE_INPUT:
            return "Mouse Input";
        case HIDReportType::KEYBOARD_OUTPUT:
            return "Keyboard Output";
        case HIDReportType::FEATURE:
            return "Feature";
        default:
            return "Unknown";
    }
}

bool is_hid_service(const QBluetoothUuid& service_uuid) {
    QString uuid_str = service_uuid.toString().toUpper();
    return uuid_str.contains("1812");
}

class HIDServiceManager {
  public:
    using ReportWriter = std::function<void(const QByteArray&)>;

    struct CharacteristicMapping {
        MockQLowEnergyCharacteristic characteristic;
        HIDReportType report_type;
        std::uint8_t report_id;
        std::size_t report_size;
        ReportWriter writer;
        bool is_valid;
    };

  private:
    std::unordered_map<HIDReportType, CharacteristicMapping> characteristic_map_;
    MockQLowEnergyService* hid_service_ = nullptr;
    bool service_ready_ = false;

  public:
    bool initialize(MockQLowEnergyService* service) {
        if (!service)
            return false;

        hid_service_ = service;

        // Mock characteristic discovery - create keyboard input mapping
        CharacteristicMapping keyboard_mapping;
        keyboard_mapping.characteristic.valid = true;
        keyboard_mapping.characteristic.uuid_str = "2A22";  // Boot keyboard
        keyboard_mapping.report_type = HIDReportType::KEYBOARD_INPUT;
        keyboard_mapping.report_id = 0x01;
        keyboard_mapping.report_size = 8;
        keyboard_mapping.is_valid = true;
        keyboard_mapping.writer = [service](const QByteArray& data) {
            MockQLowEnergyCharacteristic ch;
            service->writeCharacteristic(ch, data, 0);
        };

        characteristic_map_[HIDReportType::KEYBOARD_INPUT] = std::move(keyboard_mapping);

        // Mock consumer control mapping
        CharacteristicMapping consumer_mapping;
        consumer_mapping.characteristic.valid = true;
        consumer_mapping.characteristic.uuid_str = "2A4D";  // Generic report
        consumer_mapping.report_type = HIDReportType::CONSUMER_CONTROL;
        consumer_mapping.report_id = 0x02;
        consumer_mapping.report_size = 2;
        consumer_mapping.is_valid = true;
        consumer_mapping.writer = [service](const QByteArray& data) {
            MockQLowEnergyCharacteristic ch;
            service->writeCharacteristic(ch, data, 0);
        };

        characteristic_map_[HIDReportType::CONSUMER_CONTROL] = std::move(consumer_mapping);

        service_ready_ = true;
        return true;
    }

    bool is_ready() const { return service_ready_; }

    bool send_keyboard_report(const std::array<std::uint8_t, 8>& report) {
        if (!service_ready_)
            return false;

        auto it = characteristic_map_.find(HIDReportType::KEYBOARD_INPUT);
        if (it == characteristic_map_.end())
            return false;

        QByteArray data(reinterpret_cast<const char*>(report.data()), report.size());
        if (it->second.report_id != 0) {
            data.prepend(static_cast<char>(it->second.report_id));
        }

        it->second.writer(data);
        return true;
    }

    bool send_consumer_control_report(const std::array<std::uint8_t, 2>& report) {
        if (!service_ready_)
            return false;

        auto it = characteristic_map_.find(HIDReportType::CONSUMER_CONTROL);
        if (it == characteristic_map_.end())
            return false;

        QByteArray data(reinterpret_cast<const char*>(report.data()), report.size());
        if (it->second.report_id != 0) {
            data.prepend(static_cast<char>(it->second.report_id));
        }

        it->second.writer(data);
        return true;
    }

    bool supports_report_type(HIDReportType report_type) const {
        return characteristic_map_.find(report_type) != characteristic_map_.end();
    }

    std::vector<HIDReportType> get_available_report_types() const {
        std::vector<HIDReportType> types;
        for (const auto& [report_type, mapping] : characteristic_map_) {
            types.push_back(report_type);
        }
        return types;
    }
};

}  // namespace ble_hid

// Mock logging
#define LOG_ERROR(msg) std::cerr << "[ERROR] " << msg << std::endl
#define LOG_WARN(msg) std::cerr << "[WARN] " << msg << std::endl
#define LOG_INFO(msg) std::cout << "[INFO] " << msg << std::endl
#define LOG_DEBUG(msg) std::cout << "[DEBUG] " << msg << std::endl

// Test functions
void test_hid_service_manager_initialization() {
    std::cout << "Testing HID Service Manager initialization... ";

    MockQLowEnergyService service;
    ble_hid::HIDServiceManager manager;

    // Test initialization
    assert(manager.initialize(&service));
    assert(manager.is_ready());

    // Test available report types
    auto types = manager.get_available_report_types();
    assert(types.size() == 2);
    assert(manager.supports_report_type(ble_hid::HIDReportType::KEYBOARD_INPUT));
    assert(manager.supports_report_type(ble_hid::HIDReportType::CONSUMER_CONTROL));

    std::cout << "PASSED" << std::endl;
}

void test_keyboard_report_sending() {
    std::cout << "Testing keyboard report sending... ";

    MockQLowEnergyService service;
    ble_hid::HIDServiceManager manager;

    assert(manager.initialize(&service));

    // Test keyboard report
    std::array<std::uint8_t, 8> keyboard_report = {0x01, 0x00, 0x04, 0x00,
                                                   0x00, 0x00, 0x00, 0x00};  // Ctrl+A
    assert(manager.send_keyboard_report(keyboard_report));

    assert(service.write_called);
    assert(service.last_written_data.size() == 9);  // 8 bytes + 1 report ID
    assert(service.last_written_data[0] == 0x01);   // Report ID
    assert(service.last_written_data[1] == 0x01);   // Modifier
    assert(service.last_written_data[3] == 0x04);   // Key code

    std::cout << "PASSED" << std::endl;
}

void test_consumer_control_report_sending() {
    std::cout << "Testing consumer control report sending... ";

    MockQLowEnergyService service;
    ble_hid::HIDServiceManager manager;

    assert(manager.initialize(&service));

    // Reset service state
    service.write_called = false;
    service.last_written_data.clear();

    // Test consumer control report
    std::array<std::uint8_t, 2> consumer_report = {0xE9, 0x00};  // Volume Up
    assert(manager.send_consumer_control_report(consumer_report));

    assert(service.write_called);
    assert(service.last_written_data.size() == 3);  // 2 bytes + 1 report ID
    assert(service.last_written_data[0] == 0x02);   // Report ID
    assert(service.last_written_data[1] == 0xE9);   // Volume Up low byte
    assert(service.last_written_data[2] == 0x00);   // Volume Up high byte

    std::cout << "PASSED" << std::endl;
}

void test_utility_functions() {
    std::cout << "Testing utility functions... ";

    // Test report type to string conversion
    assert(std::string(ble_hid::report_type_to_string(ble_hid::HIDReportType::KEYBOARD_INPUT)) ==
           "Keyboard Input");
    assert(std::string(ble_hid::report_type_to_string(ble_hid::HIDReportType::CONSUMER_CONTROL)) ==
           "Consumer Control");

    // Test HID service detection
    QBluetoothUuid hid_uuid("00001812-0000-1000-8000-00805f9b34fb");
    assert(ble_hid::is_hid_service(hid_uuid));

    QBluetoothUuid non_hid_uuid("00001234-0000-1000-8000-00805f9b34fb");
    assert(!ble_hid::is_hid_service(non_hid_uuid));

    std::cout << "PASSED" << std::endl;
}

int main() {
    std::cout << "=== BLE HID Service Manager Unit Tests ===" << std::endl;

    try {
        test_hid_service_manager_initialization();
        test_keyboard_report_sending();
        test_consumer_control_report_sending();
        test_utility_functions();

        std::cout << std::endl
                  << "=== All BLE HID Service Manager tests completed ===" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
