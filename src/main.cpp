// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2025 Dharun A P

#include "hid_keycodes.hpp"    // HID keyboard mappings and state management
#include "device_manager.hpp"  // Device enumeration and hot-plug support
#include "args.hpp"            // Command-line argument parsing
#include "logger.hpp"          // Logging utilities
#include "version.hpp"         // Version information
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothDeviceInfo>
#include <QBluetoothUuid>
#include <QByteArray>
#include <QCoreApplication>
#include <QLowEnergyController>
#include <QLowEnergyService>
#include <QThread>
#include <QTimer>
#include <array>
#include <atomic>     // Add missing atomic header
#include <csignal>
#include <functional> // Add missing functional header
#include <iostream>
#include <libevdev/libevdev.h>
#include <poll.h>
#include <vector>

// ---------------------------------------------------------------------------
//  Constants & Global state  
// ---------------------------------------------------------------------------
std::atomic<bool> g_running{true};

// Global options (will be set from command line)
namespace {
    args::Options g_options;
}

void signal_handler(int signum) {
    LOG_INFO("Caught signal " + std::to_string(signum) + ", exiting...");
    g_running = false;
}

// ---------------------------------------------------------------------------
//  Helper functions
// ---------------------------------------------------------------------------

/**
 * @brief Create a function to write HID reports to BLE characteristic
 */
auto make_report_writer(QLowEnergyService *service, QLowEnergyCharacteristic ch) {
    return [service, ch](const std::array<uint8_t, 8> &report) {
        if (!service || !ch.isValid())
            return;
        QByteArray data(reinterpret_cast<const char *>(report.data()), 8);
        service->writeCharacteristic(ch, data, QLowEnergyService::WriteWithoutResponse);
    };
}

// ---------------------------------------------------------------------------
//  Main
// ---------------------------------------------------------------------------
int main(int argc, char *argv[]) {
    // Parse command line arguments
    args::ArgumentParser parser(argc, argv);
    auto options = parser.parse();
    
    if (!options) {
        return 1; // Parse error already reported
    }
    
    g_options = *options;
    
    // Handle help and version
    if (g_options.show_help) {
        parser.show_help();
        return 0;
    }
    
    if (g_options.show_version) {
        parser.show_version();
        return 0;
    }
    
    // Configure logging
    logging::Logger::set_level(g_options.log_level);
    logging::Logger::enable_timestamps(g_options.verbose);
    
    if (g_options.verbose) {
        LOG_INFO("Starting " + std::string(version::APP_NAME) + " " + version::get_version());
        LOG_DEBUG("Verbose logging enabled");
        LOG_DEBUG("Scan timeout: " + std::to_string(g_options.scan_timeout) + "ms");
        LOG_DEBUG("Poll interval: " + std::to_string(g_options.poll_interval) + "ms");
    }

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // ------------------ Initialize device management ------------------
    device::KeyboardManager keyboard_manager;
    if (!keyboard_manager.is_valid()) {
        LOG_ERROR("Failed to initialize device monitoring");
        return 1;
    }

    LOG_INFO("Found " + std::to_string(keyboard_manager.device_count()) + " keyboard(s)");
    LOG_DEBUG("Monitoring keyboards (hot-plug supported)...");

    hid::KeyboardState kb_state;

    // ------------------ Qt setup ------------------
    QCoreApplication app(argc, argv);
    QBluetoothDeviceDiscoveryAgent discoveryAgent;
    discoveryAgent.setLowEnergyDiscoveryTimeout(g_options.scan_timeout);

    // Handle list devices option
    if (g_options.list_devices) {
        LOG_INFO("Scanning for BLE devices...");
        // We'll just start discovery and exit after listing
    }

    QList<QBluetoothDeviceInfo> foundDevices;
    QLowEnergyController *controller = nullptr;
    QLowEnergyService *service = nullptr;
    QLowEnergyCharacteristic targetChar;
    std::function<void(const std::array<uint8_t, 8> &)> sendReport;

    // ----- Device discovery -----
    QObject::connect(
        &discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
        [&](const QBluetoothDeviceInfo &info) {
            LOG_INFO("Found device " + std::to_string(foundDevices.size()) + ": " + 
                    info.name().toStdString() + " [" + info.address().toString().toStdString() + "]");
            foundDevices.append(info);
        });

    QObject::connect(
        &discoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished, [&]() {
            if (g_options.list_devices) {
                LOG_INFO("BLE device discovery completed. Found " + std::to_string(foundDevices.size()) + " devices");
                app.quit();
                return;
            }
            
            if (foundDevices.empty()) {
                LOG_ERROR("No BLE devices found – exiting.");
                app.quit();
                return;
            }
            
            int index = 0;
            
            // Check if target device specified
            if (!g_options.target_device.empty()) {
                bool found = false;
                for (int i = 0; i < foundDevices.size(); ++i) {
                    if (foundDevices[i].address().toString().toStdString() == g_options.target_device ||
                        foundDevices[i].name().toStdString() == g_options.target_device) {
                        index = i;
                        found = true;
                        LOG_INFO("Found target device: " + g_options.target_device);
                        break;
                    }
                }
                if (!found) {
                    LOG_ERROR("Target device not found: " + g_options.target_device);
                    app.quit();
                    return;
                }
            } else {
                LOG_INFO("Discovery complete. Choose device number: ");
                std::cin >> index;
                if (index < 0 || index >= foundDevices.size()) {
                    LOG_ERROR("Invalid device index");
                    app.quit();
                    return;
                }
            }

            const QBluetoothDeviceInfo &device = foundDevices[index];
            controller = QLowEnergyController::createCentral(device);
            
            LOG_INFO("Connecting to device: " + device.name().toStdString());

            QObject::connect(controller, &QLowEnergyController::connected, [&]() {
                LOG_INFO("Connected. Discovering services...");
                controller->discoverServices();
            });
            QObject::connect(controller, &QLowEnergyController::disconnected,
                             [&]() {
                                 LOG_WARN("Disconnected from BLE device");
                                 g_running = false;
                                 app.quit();
                             });

            QObject::connect(controller, &QLowEnergyController::serviceDiscovered,
                             [&](const QBluetoothUuid &uuid) {
                                 LOG_DEBUG("Service discovered: " + uuid.toString().toStdString());
                             });

            QObject::connect(
                controller, &QLowEnergyController::discoveryFinished, [&]() {
                    LOG_DEBUG("Service discovery finished");
                    // Pick first service and search for writable char
                    for (const QBluetoothUuid &uuid : controller->services()) {
                        service = controller->createServiceObject(uuid);
                        if (!service)
                            continue;

                        QObject::connect(
                            service, &QLowEnergyService::stateChanged,
                            [&](QLowEnergyService::ServiceState s) {
                                if (s != QLowEnergyService::RemoteServiceDiscovered)
                                    return;
                                for (const auto &c : service->characteristics()) {
                                    if (c.properties() &
                                        (QLowEnergyCharacteristic::Write |
                                         QLowEnergyCharacteristic::WriteNoResponse)) {
                                        targetChar = c;
                                        sendReport = make_report_writer(service, targetChar);
                                        LOG_INFO("✔ Found writable characteristic: " + c.uuid().toString().toStdString());
                                        LOG_INFO("Ready! Start typing – Ctrl+C to quit.");
                                    }
                                }
                            });
                        service->discoverDetails();
                        if (targetChar.isValid())
                            break;
                    }
                    if (!targetChar.isValid()) {
                        LOG_ERROR("No writable characteristic found");
                        app.quit();
                    }
                });
            controller->connectToDevice();
        });

    discoveryAgent.start();

    // ------------------ Input processing loop ------------------
    QTimer pollTimer;
    pollTimer.setInterval(g_options.poll_interval);
    QObject::connect(&pollTimer, &QTimer::timeout, [&] {
        if (!g_running || !sendReport)
            return;

        // Update device list (handle hot-plug events)
        if (keyboard_manager.update_devices() && g_options.verbose) {
            LOG_DEBUG("Device list updated");
        }

        // Get poll file descriptors
        std::vector<int> fds = keyboard_manager.get_poll_fds();
        if (fds.empty())
            return;

        // Create pollfd structures
        std::vector<struct pollfd> pfds;
        pfds.reserve(fds.size());
        for (int fd : fds) {
            pfds.push_back({fd, POLLIN, 0});
        }

        if (poll(pfds.data(), pfds.size(), 0) <= 0)
            return; // non-blocking

        // Process keyboard events
        const auto& keyboards = keyboard_manager.keyboards();
        for (size_t i = 0; i < keyboards.size(); ++i) {
            if (i >= pfds.size() || !(pfds[i].revents & POLLIN))
                continue;

            input_event ev{};
            while (libevdev_next_event(keyboards[i].evdev(), LIBEVDEV_READ_FLAG_NORMAL, &ev) == 0) {
                if (ev.type != EV_KEY)
                    continue;

                if (g_options.verbose) {
                    LOG_DEBUG("Key event: code=" + std::to_string(ev.code) + 
                             " value=" + std::to_string(ev.value) + 
                             " from " + keyboards[i].name());
                }

                switch (ev.value) {
                case 1: // key down
                case 2: // auto-repeat
                    if (hid::apply_key_event(kb_state, ev.code, ev.value)) {
                        sendReport(kb_state.get_report());
                    }
                    break;
                case 0: // key release
                    [[maybe_unused]] bool changed = hid::apply_key_event(kb_state, ev.code, ev.value);
                    // Send all-zero release report to stop the key
                    sendReport({0, 0, 0, 0, 0, 0, 0, 0});
                    break;
                }
            }
        }
    });
    pollTimer.start();

    int ret = app.exec();
    return ret;
}