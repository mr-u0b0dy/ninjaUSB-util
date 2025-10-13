/**
 * @file ble_hid_service_manager.cpp
 * @brief Implementation of BLE HID Service Manager for Universal HID Support
 * @author Assistant
 * @date 2025
 * @license SPDX-License-Identifier: Apache-2.0
 */

#include "ble_hid_service_manager.hpp"

#include <QBluetoothUuid>
#include <QLowEnergyDescriptor>

#include "logger.hpp"

namespace ble_hid {

// ---------------------------------------------------------------------------
//  HIDServiceManager Implementation
// ---------------------------------------------------------------------------

bool HIDServiceManager::initialize(QLowEnergyService* service) {
    if (!service) {
        LOG_ERROR("Invalid HID service pointer provided");
        return false;
    }

    // Verify this is a HID or custom service
    if (!is_hid_service(service->serviceUuid())) {
        const auto uuid_str = service->serviceUuid().toString().toStdString();
        if (uuid_str != std::string(CUSTOM_INPUT_SERVICE_UUID)) {
            LOG_WARN("Service is neither standard HID nor custom input service: " + uuid_str);
        } else {
            LOG_INFO("Initializing custom input service: " + uuid_str);
        }
    }

    hid_service_ = service;

    // Discover and map characteristics
    discover_characteristics(service);

    // Check if we have at least one usable characteristic
    service_ready_ = !characteristic_map_.empty();

    if (service_ready_) {
        LOG_INFO("HID Service Manager initialized successfully with " +
                 std::to_string(characteristic_map_.size()) + " characteristic(s)");

        // Log available report types
        for (const auto& [report_type, mapping] : characteristic_map_) {
            LOG_DEBUG("Available report type: " + std::string(report_type_to_string(report_type)) +
                      " [UUID: " + mapping.characteristic.uuid().toString().toStdString() + "]");
        }
    } else {
        LOG_ERROR("No usable HID characteristics found in service");
    }

    return service_ready_;
}

bool HIDServiceManager::send_keyboard_report(const std::array<std::uint8_t, 8>& report) {
    if (!service_ready_) {
        LOG_WARN("HID service not ready, cannot send keyboard report");
        return false;
    }

    auto it = characteristic_map_.find(HIDReportType::KEYBOARD_INPUT);
    if (it == characteristic_map_.end()) {
        LOG_WARN("No keyboard input characteristic available");
        return false;
    }

    // Create QByteArray from report data
    QByteArray data(reinterpret_cast<const char*>(report.data()), report.size());

    // Add report ID if characteristic expects it
    if (it->second.report_id != 0) {
        data.prepend(static_cast<char>(it->second.report_id));
    }

    // Log target characteristic for verification
    LOG_INFO("Writing keyboard report to characteristic UUID: " +
             it->second.characteristic.uuid().toString().toStdString());

    // Send the report
    it->second.writer(data);
    return true;
}

bool HIDServiceManager::send_consumer_control_report(const std::array<std::uint8_t, 2>& report) {
    if (!service_ready_) {
        LOG_WARN("HID service not ready, cannot send consumer control report");
        return false;
    }

    auto it = characteristic_map_.find(HIDReportType::CONSUMER_CONTROL);
    if (it == characteristic_map_.end()) {
        LOG_WARN("No consumer control characteristic available");
        return false;
    }

    // Create QByteArray from report data
    QByteArray data(reinterpret_cast<const char*>(report.data()), report.size());

    // Add report ID if characteristic expects it
    if (it->second.report_id != 0) {
        data.prepend(static_cast<char>(it->second.report_id));
    }

    // Log target characteristic for verification
    LOG_INFO("Writing consumer control report to characteristic UUID: " +
             it->second.characteristic.uuid().toString().toStdString());

    // Send the report
    it->second.writer(data);
    return true;
}

bool HIDServiceManager::send_report(HIDReportType report_type, const QByteArray& data) {
    if (!service_ready_) {
        LOG_WARN("HID service not ready, cannot send report");
        return false;
    }

    auto it = characteristic_map_.find(report_type);
    if (it == characteristic_map_.end()) {
        LOG_WARN("No characteristic available for report type: " +
                 std::string(report_type_to_string(report_type)));
        return false;
    }

    QByteArray formatted_data = data;

    // Add report ID if characteristic expects it
    if (it->second.report_id != 0) {
        formatted_data.prepend(static_cast<char>(it->second.report_id));
    }

    // Send the report
    it->second.writer(formatted_data);
    return true;
}

std::optional<HIDServiceManager::CharacteristicMapping>
HIDServiceManager::get_characteristic_mapping(HIDReportType report_type) const {
    auto it = characteristic_map_.find(report_type);
    if (it != characteristic_map_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<HIDReportType> HIDServiceManager::get_available_report_types() const {
    std::vector<HIDReportType> types;
    types.reserve(characteristic_map_.size());

    for (const auto& [report_type, mapping] : characteristic_map_) {
        types.push_back(report_type);
    }

    return types;
}

bool HIDServiceManager::supports_report_type(HIDReportType report_type) const {
    return characteristic_map_.find(report_type) != characteristic_map_.end();
}

void HIDServiceManager::discover_characteristics(QLowEnergyService* service) {
    characteristic_map_.clear();

    LOG_DEBUG("Discovering HID characteristics for service: " +
              service->serviceUuid().toString().toStdString());

    for (const auto& characteristic : service->characteristics()) {
    LOG_DEBUG("Analyzing characteristic: " + characteristic.uuid().toString().toStdString());
    LOG_DEBUG("  Properties: " + std::to_string(characteristic.properties()));

        // Check if characteristic supports writing
        if (!(characteristic.properties() &
              (QLowEnergyCharacteristic::Write | QLowEnergyCharacteristic::WriteNoResponse))) {
            LOG_DEBUG("Characteristic doesn't support writing, skipping");
            continue;
        }

        // If this is the custom command characteristic, treat as keyboard input path
        std::optional<HIDReportType> report_type;
        QBluetoothUuid char_uuid = characteristic.uuid();
        QBluetoothUuid custom_cmd_uuid(QString::fromLatin1(CUSTOM_COMMAND_CHAR_UUID));

        const bool is_custom_cmd_char = (char_uuid == custom_cmd_uuid);
        if (is_custom_cmd_char) {
            report_type = HIDReportType::KEYBOARD_INPUT;  // route keyboard reports here
            LOG_INFO("Identified custom command characteristic for keyboard input [UUID: " +
                     char_uuid.toString().toStdString() + "]");
        } else {
            // Otherwise use generic identification logic
            report_type = identify_report_type(characteristic);
        }
        if (!report_type) {
            LOG_DEBUG("Could not identify report type for characteristic");
            continue;
        }

        // Validate characteristic for the identified report type
        if (!validate_characteristic(characteristic, *report_type)) {
            LOG_DEBUG("Characteristic validation failed for report type: " +
                      std::string(report_type_to_string(*report_type)));
            continue;
        }

        // Create characteristic mapping
        CharacteristicMapping mapping;
        mapping.characteristic = characteristic;
        mapping.report_type = *report_type;
        mapping.writer = create_report_writer(service, characteristic, *report_type);
        mapping.is_valid = true;

        // Set report parameters based on type
        switch (*report_type) {
            case HIDReportType::KEYBOARD_INPUT:
                // For the custom command characteristic, send raw 8-byte keyboard report without
                // a Report ID prefix to match firmware expectations.
                if (is_custom_cmd_char) {
                    mapping.report_id = 0;
                    mapping.report_size = KEYBOARD_REPORT_DESC.report_size;  // 8 bytes
                } else {
                    mapping.report_id = KEYBOARD_REPORT_DESC.report_id;  // prepend 0x01
                    mapping.report_size = KEYBOARD_REPORT_DESC.report_size;
                }
                break;
            case HIDReportType::CONSUMER_CONTROL:
                mapping.report_id = CONSUMER_CONTROL_REPORT_DESC.report_id;
                mapping.report_size = CONSUMER_CONTROL_REPORT_DESC.report_size;
                break;
            default:
                mapping.report_id = 0;
                mapping.report_size = 0;
                break;
        }

        characteristic_map_[*report_type] = std::move(mapping);

        LOG_INFO("Mapped characteristic for " + std::string(report_type_to_string(*report_type)) +
                 " [UUID: " + characteristic.uuid().toString().toStdString() + "]");
    }
}

HIDServiceManager::ReportWriter
HIDServiceManager::create_report_writer(QLowEnergyService* service,
                                        QLowEnergyCharacteristic characteristic,
                                        HIDReportType report_type) {
    return [service, characteristic, report_type](const QByteArray& data) {
        if (!service || !characteristic.isValid()) {
            LOG_WARN("Invalid service or characteristic, skipping " +
                     std::string(report_type_to_string(report_type)) + " report");
            return;
        }

        // Determine best write mode supported by the characteristic
        const auto props = characteristic.properties();
        const bool supports_wnr = (props & QLowEnergyCharacteristic::WriteNoResponse);
        const bool supports_wr = (props & QLowEnergyCharacteristic::Write);

        QLowEnergyService::WriteMode mode = QLowEnergyService::WriteWithoutResponse;
        if (supports_wnr) {
            mode = QLowEnergyService::WriteWithoutResponse;
        } else if (supports_wr) {
            mode = QLowEnergyService::WriteWithResponse;
        } else {
            LOG_WARN("Characteristic does not support write operations: " +
                     characteristic.uuid().toString().toStdString());
            return;
        }

        service->writeCharacteristic(characteristic, data, mode);

        const std::string mode_str =
            (mode == QLowEnergyService::WriteWithoutResponse) ? "WriteWithoutResponse"
                                                              : "WriteWithResponse";
        LOG_DEBUG(std::string("Sent ") + report_type_to_string(report_type) +
                  " report to [UUID: " +
                  characteristic.uuid().toString().toStdString() + "] (" +
                  std::to_string(data.size()) + " bytes, mode=" + mode_str + ")");
    };
}

std::optional<HIDReportType>
HIDServiceManager::identify_report_type(const QLowEnergyCharacteristic& characteristic) {
    QString uuid = characteristic.uuid().toString().toUpper();

    // Check for boot keyboard input report
    if (uuid.contains("2A22")) {
        return HIDReportType::KEYBOARD_INPUT;
    }

    // Check for standard report characteristic
    if (uuid.contains("2A4D")) {
        // For generic report characteristics, we need to check descriptors
        // or use heuristics based on the service context

        // Check if there are descriptors that might indicate report type
        for (const auto& descriptor : characteristic.descriptors()) {
            QString desc_uuid = descriptor.uuid().toString().toUpper();

            // Check for Report Reference descriptor (0x2908)
            if (desc_uuid.contains("2908")) {
                // Would need to read descriptor value to determine report type
                // For now, fall back to heuristics
                break;
            }
        }

        // Use characteristic name or other heuristics if available
        QString name = characteristic.name().toUpper();
        if (name.contains("KEYBOARD") || name.contains("KEY")) {
            return HIDReportType::KEYBOARD_INPUT;
        }
        if (name.contains("CONSUMER") || name.contains("MEDIA")) {
            return HIDReportType::CONSUMER_CONTROL;
        }

        // Default to keyboard input for first generic characteristic
        return HIDReportType::KEYBOARD_INPUT;
    }

    // Look for any characteristic that might be writable and HID-related
    // This is a fallback for non-standard implementations
    if (characteristic.properties() &
        (QLowEnergyCharacteristic::Write | QLowEnergyCharacteristic::WriteNoResponse)) {
        // Assume keyboard input as the most common use case
        return HIDReportType::KEYBOARD_INPUT;
    }

    return std::nullopt;
}

bool HIDServiceManager::validate_characteristic(const QLowEnergyCharacteristic& characteristic,
                                                HIDReportType report_type) {
    // Check if characteristic is valid
    if (!characteristic.isValid()) {
        return false;
    }

    // Check if characteristic supports required properties
    auto properties = characteristic.properties();
    if (!(properties &
          (QLowEnergyCharacteristic::Write | QLowEnergyCharacteristic::WriteNoResponse))) {
        return false;
    }

    // Additional validation based on report type could be added here
    switch (report_type) {
        case HIDReportType::KEYBOARD_INPUT:
        case HIDReportType::CONSUMER_CONTROL:
            // These are input reports, so Write or WriteNoResponse is sufficient
            return true;
        case HIDReportType::KEYBOARD_OUTPUT:
            // Output reports might have different requirements
            return true;
        default:
            return true;
    }
}

// ---------------------------------------------------------------------------
//  Utility Functions Implementation
// ---------------------------------------------------------------------------

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
    const QBluetoothUuid standard_hid(QString::fromLatin1(HID_SERVICE_UUID));
    const QBluetoothUuid custom_service(QString::fromLatin1(CUSTOM_INPUT_SERVICE_UUID));
    return (service_uuid == standard_hid) || (service_uuid == custom_service);
}

QByteArray create_formatted_report(std::uint8_t report_id, const QByteArray& data) {
    QByteArray formatted;
    formatted.reserve(data.size() + 1);

    if (report_id != 0) {
        formatted.append(static_cast<char>(report_id));
    }
    formatted.append(data);

    return formatted;
}

}  // namespace ble_hid
