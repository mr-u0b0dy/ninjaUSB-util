/**
 * @file ble_hid_service_manager.cpp
 * @brief Implementation of BLE HID Service Manager for Universal HID Support
 * @author Assistant
 * @date 2025
 * @license SPDX-License-Identifier: Apache-2.0
 */

#include "ble_hid_service_manager.hpp"
#include "logger.hpp"
#include <QBluetoothUuid>
#include <QLowEnergyDescriptor>

namespace ble_hid {

// ---------------------------------------------------------------------------
//  HIDServiceManager Implementation
// ---------------------------------------------------------------------------

bool HIDServiceManager::initialize(QLowEnergyService* service) {
    if (!service) {
        LOG_ERROR("Invalid HID service pointer provided");
        return false;
    }

    // Verify this is actually a HID service
    if (!is_hid_service(service->serviceUuid())) {
        LOG_WARN("Service is not a standard HID service: " + 
                 service->serviceUuid().toString().toStdString());
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
    
    LOG_DEBUG("Discovering HID characteristics...");
    
    for (const auto& characteristic : service->characteristics()) {
        LOG_DEBUG("Analyzing characteristic: " + characteristic.uuid().toString().toStdString());
        
        // Check if characteristic supports writing
        if (!(characteristic.properties() & (QLowEnergyCharacteristic::Write | 
                                           QLowEnergyCharacteristic::WriteNoResponse))) {
            LOG_DEBUG("Characteristic doesn't support writing, skipping");
            continue;
        }

        // Try to identify the report type
        auto report_type = identify_report_type(characteristic);
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
                mapping.report_id = KEYBOARD_REPORT_DESC.report_id;
                mapping.report_size = KEYBOARD_REPORT_DESC.report_size;
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

HIDServiceManager::ReportWriter HIDServiceManager::create_report_writer(
    QLowEnergyService* service,
    QLowEnergyCharacteristic characteristic,
    HIDReportType report_type) {
    
    return [service, characteristic, report_type](const QByteArray& data) {
        if (!service || !characteristic.isValid()) {
            LOG_WARN("Invalid service or characteristic, skipping " + 
                     std::string(report_type_to_string(report_type)) + " report");
            return;
        }

        // Use WriteWithoutResponse for better performance
        service->writeCharacteristic(characteristic, data, QLowEnergyService::WriteWithoutResponse);
        
        LOG_DEBUG("Sent " + std::string(report_type_to_string(report_type)) + 
                  " report (" + std::to_string(data.size()) + " bytes)");
    };
}

std::optional<HIDReportType> HIDServiceManager::identify_report_type(
    const QLowEnergyCharacteristic& characteristic) {
    
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
    if (characteristic.properties() & (QLowEnergyCharacteristic::Write | 
                                     QLowEnergyCharacteristic::WriteNoResponse)) {
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
    if (!(properties & (QLowEnergyCharacteristic::Write | 
                       QLowEnergyCharacteristic::WriteNoResponse))) {
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
    QString uuid_str = service_uuid.toString().toUpper();
    
    // Check for standard HID service UUID (0x1812)
    return uuid_str.contains("1812") || uuid_str.contains(QString(HID_SERVICE_UUID).toUpper());
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

} // namespace ble_hid
