/**
 * @file ble_hid_service_manager.hpp
 * @brief BLE HID Service Manager for Universal HID Support
 * @author Assistant
 * @date 2025
 * @license SPDX-License-Identifier: Apache-2.0
 *
 * This module provides comprehensive BLE HID service management with support for
 * multiple HID report types routed to specific characteristics according to
 * the HID over BLE specification.
 */

#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <QBluetoothUuid>
#include <QByteArray>
#include <QLowEnergyCharacteristic>
#include <QLowEnergyService>
#include <unordered_map>

/**
 * @namespace ble_hid
 * @brief BLE HID service management and characteristic mapping
 */
namespace ble_hid {

// ---------------------------------------------------------------------------
//  HID Service and Characteristic UUIDs
// ---------------------------------------------------------------------------

/**
 * @brief Standard Bluetooth HID Service UUID
 */
constexpr const char* HID_SERVICE_UUID = "00001812-0000-1000-8000-00805f9b34fb";

/**
 * @brief HID Information Characteristic UUID
 */
constexpr const char* HID_INFORMATION_UUID = "00002a4a-0000-1000-8000-00805f9b34fb";

/**
 * @brief HID Control Point Characteristic UUID
 */
constexpr const char* HID_CONTROL_POINT_UUID = "00002a4c-0000-1000-8000-00805f9b34fb";

/**
 * @brief Report Map Characteristic UUID
 */
constexpr const char* REPORT_MAP_UUID = "00002a4b-0000-1000-8000-00805f9b34fb";

/**
 * @brief Report Characteristic UUID (Input/Output/Feature)
 */
constexpr const char* REPORT_UUID = "00002a4d-0000-1000-8000-00805f9b34fb";

/**
 * @brief Boot Keyboard Input Report Characteristic UUID
 */
constexpr const char* BOOT_KEYBOARD_INPUT_REPORT_UUID = "00002a22-0000-1000-8000-00805f9b34fb";

/**
 * @brief Boot Keyboard Output Report Characteristic UUID
 */
constexpr const char* BOOT_KEYBOARD_OUTPUT_REPORT_UUID = "00002a32-0000-1000-8000-00805f9b34fb";

// ---------------------------------------------------------------------------
//  HID Report Types and Identifiers
// ---------------------------------------------------------------------------

/**
 * @brief HID Report Types according to HID specification
 */
enum class HIDReportType : std::uint8_t {
    KEYBOARD_INPUT = 0x01,    //!< Standard keyboard input report
    CONSUMER_CONTROL = 0x02,  //!< Consumer control (media keys) input report
    MOUSE_INPUT = 0x03,       //!< Mouse input report (future extension)
    KEYBOARD_OUTPUT = 0x11,   //!< Keyboard output report (LED status)
    FEATURE = 0x21            //!< Feature report
};

/**
 * @brief HID Report Descriptor IDs for different report types
 */
struct HIDReportDescriptor {
    std::uint8_t report_id;
    HIDReportType report_type;
    std::size_t report_size;
    const char* description;
};

// Standard HID report descriptors
constexpr HIDReportDescriptor KEYBOARD_REPORT_DESC = {0x01, HIDReportType::KEYBOARD_INPUT, 8,
                                                      "Keyboard Input Report"};

constexpr HIDReportDescriptor CONSUMER_CONTROL_REPORT_DESC = {0x02, HIDReportType::CONSUMER_CONTROL,
                                                              2, "Consumer Control Input Report"};

// ---------------------------------------------------------------------------
//  HID Service Manager Class
// ---------------------------------------------------------------------------

/**
 * @brief Manages BLE HID services and characteristic routing
 *
 * This class provides a comprehensive interface for managing HID over BLE
 * communication with support for multiple report types routed to appropriate
 * characteristics based on HID usage and report type.
 */
class HIDServiceManager {
  public:
    /**
     * @brief Function type for sending HID reports to characteristics
     */
    using ReportWriter = std::function<void(const QByteArray&)>;

    /**
     * @brief Characteristic mapping information
     */
    struct CharacteristicMapping {
        QLowEnergyCharacteristic characteristic;
        HIDReportType report_type;
        std::uint8_t report_id;
        std::size_t report_size;
        ReportWriter writer;
        bool is_valid;
    };

  private:
    std::unordered_map<HIDReportType, CharacteristicMapping> characteristic_map_;
    QLowEnergyService* hid_service_ = nullptr;
    bool service_ready_ = false;

  public:
    /**
     * @brief Constructor
     */
    HIDServiceManager() = default;

    /**
     * @brief Destructor
     */
    ~HIDServiceManager() = default;

    // Disable copy constructor and assignment
    HIDServiceManager(const HIDServiceManager&) = delete;
    HIDServiceManager& operator=(const HIDServiceManager&) = delete;

    /**
     * @brief Initialize HID service with discovered BLE service
     * @param service Pointer to the discovered HID service
     * @return true if initialization successful, false otherwise
     */
    bool initialize(QLowEnergyService* service);

    /**
     * @brief Check if HID service is ready for communication
     * @return true if service is initialized and ready
     */
    bool is_ready() const noexcept { return service_ready_; }

    /**
     * @brief Send keyboard input report (8-byte standard keyboard report)
     * @param report 8-byte keyboard HID report
     * @return true if sent successfully, false otherwise
     */
    bool send_keyboard_report(const std::array<std::uint8_t, 8>& report);

    /**
     * @brief Send consumer control report (2-byte media key report)
     * @param report 2-byte consumer control HID report
     * @return true if sent successfully, false otherwise
     */
    bool send_consumer_control_report(const std::array<std::uint8_t, 2>& report);

    /**
     * @brief Send raw HID report to appropriate characteristic
     * @param report_type Type of HID report
     * @param data Raw report data
     * @return true if sent successfully, false otherwise
     */
    bool send_report(HIDReportType report_type, const QByteArray& data);

    /**
     * @brief Get characteristic mapping for a specific report type
     * @param report_type HID report type
     * @return Optional characteristic mapping
     */
    std::optional<CharacteristicMapping>
    get_characteristic_mapping(HIDReportType report_type) const;

    /**
     * @brief Get list of available report types
     * @return Vector of available HID report types
     */
    std::vector<HIDReportType> get_available_report_types() const;

    /**
     * @brief Check if a specific report type is supported
     * @param report_type HID report type to check
     * @return true if supported, false otherwise
     */
    bool supports_report_type(HIDReportType report_type) const;

  private:
    /**
     * @brief Discover and map HID characteristics
     * @param service HID service to analyze
     */
    void discover_characteristics(QLowEnergyService* service);

    /**
     * @brief Create report writer for a characteristic
     * @param service BLE service pointer
     * @param characteristic Target characteristic
     * @param report_type HID report type
     * @return Report writer function
     */
    ReportWriter create_report_writer(QLowEnergyService* service,
                                      QLowEnergyCharacteristic characteristic,
                                      HIDReportType report_type);

    /**
     * @brief Identify HID report type from characteristic
     * @param characteristic BLE characteristic to analyze
     * @return Optional HID report type
     */
    std::optional<HIDReportType>
    identify_report_type(const QLowEnergyCharacteristic& characteristic);

    /**
     * @brief Validate HID characteristic for specific report type
     * @param characteristic BLE characteristic
     * @param report_type Expected report type
     * @return true if valid, false otherwise
     */
    bool validate_characteristic(const QLowEnergyCharacteristic& characteristic,
                                 HIDReportType report_type);
};

// ---------------------------------------------------------------------------
//  Utility Functions
// ---------------------------------------------------------------------------

/**
 * @brief Convert HID report type to string representation
 * @param report_type HID report type
 * @return String description of report type
 */
const char* report_type_to_string(HIDReportType report_type);

/**
 * @brief Check if service is a HID service
 * @param service_uuid Service UUID to check
 * @return true if HID service, false otherwise
 */
bool is_hid_service(const QBluetoothUuid& service_uuid);

/**
 * @brief Create formatted report with report ID
 * @param report_id HID report ID
 * @param data Raw report data
 * @return Formatted report with ID prefix
 */
QByteArray create_formatted_report(std::uint8_t report_id, const QByteArray& data);

}  // namespace ble_hid
