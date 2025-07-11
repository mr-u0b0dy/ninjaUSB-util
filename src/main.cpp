/**
 * @file main.cpp
 * @brief Main entry point for ninjaUSB-util - USB keyboard to BLE bridge utility
 * @author Dharun A P
 * @date 2025
 * @copyright Copyright (c) 2025 Dharun A P
 * @license SPDX-License-Identifier: Apache-2.0
 *
 * This file contains the main application logic for ninjaUSB-util, a Linux utility
 * that bridges USB keyboard input to Bluetooth Low Energy (BLE) devices. The application
 * uses an event-driven architecture to monitor keyboard input and forward it as HID
 * reports over BLE connections.
 *
 * @section Architecture Main Components
 * - Device management: Hot-plug aware keyboard monitoring via udev/libevdev
 * - BLE communication: Qt6 Bluetooth for device discovery and GATT communication
 * - Input processing: Real-time key event to HID report conversion
 * - Configuration: Command-line argument parsing and runtime options
 * - Logging: Configurable logging system with multiple verbosity levels
 *
 * @section DataFlow Data Flow
 * 1. Enumerate and monitor USB keyboards via udev
 * 2. Poll keyboard devices for input events using libevdev
 * 3. Convert Linux key events to USB HID usage codes
 * 4. Generate 8-byte HID keyboard reports
 * 5. Discover and connect to BLE devices using Qt Bluetooth
 * 6. Transmit HID reports to BLE device characteristics
 *
 * @section Threading Threading Model
 * Single-threaded event-driven architecture:
 * - Main thread handles all I/O operations
 * - Event loop polls keyboard devices and processes Qt events
 * - Atomic flags for clean signal handling and shutdown
 *
 * @section Performance Performance Considerations
 * - Direct device polling for minimal input latency
 * - Efficient O(1) HID key mapping
 * - BLE transmission without response for speed
 * - RAII resource management for reliability
 */

#include <array>
#include <atomic>  // Add missing atomic header
#include <csignal>
#include <functional>  // Add missing functional header
#include <iostream>
#include <poll.h>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothDeviceInfo>
#include <QBluetoothUuid>
#include <QByteArray>
#include <QCoreApplication>
#include <QLowEnergyController>
#include <QLowEnergyService>
#include <QThread>
#include <QTimer>
#include <vector>

#include <libevdev/libevdev.h>

#include "args.hpp"                  // Command-line argument parsing
#include "device_manager.hpp"        // Device enumeration and hot-plug support
#include "exit_hotkey_detector.hpp"  // Exit hotkey detection
#include "hid_keycodes.hpp"          // HID keyboard mappings and state management
#include "logger.hpp"                // Logging utilities
#include "version.hpp"               // Version information

//! @namespace Global application state and configuration
namespace {

//! @brief Global program configuration options set from command-line arguments
args::Options g_options;

}  // anonymous namespace

// ---------------------------------------------------------------------------
//  Global State & Signal Handling
// ---------------------------------------------------------------------------

/**
 * @brief Atomic flag controlling main application loop
 *
 * This flag is set to false by signal handlers to request clean shutdown.
 * Using atomic ensures thread-safe access from signal handlers.
 */
std::atomic<bool> g_running{true};

/**
 * @brief Signal handler for graceful shutdown
 * @param signum Signal number (SIGINT, SIGTERM, etc.)
 *
 * Handles signals differently:
 * - SIGINT (Ctrl+C): Ignored to prevent accidental exit during key capture
 * - SIGTERM: Still triggers graceful shutdown
 *
 * @note This function is called from signal context and must be signal-safe.
 *       Only async-signal-safe functions should be used here.
 */
void signal_handler(int signum) {
    if (signum == SIGINT) {
        // Ignore Ctrl+C to prevent accidental program termination during key capture
        return;
    }

    LOG_INFO("Caught signal " + std::to_string(signum) + ", exiting...");
    g_running = false;
}

// ---------------------------------------------------------------------------
//  BLE Communication Helpers
// ---------------------------------------------------------------------------

/**
 * @brief Factory function to create HID report writer for BLE characteristic
 * @param service Pointer to the BLE GATT service
 * @param ch The writable characteristic for HID reports
 * @return Lambda function that writes HID reports to the BLE characteristic
 *
 * This function returns a lambda that captures the service and characteristic
 * for writing HID keyboard reports. The lambda validates the service and
 * characteristic before each write operation.
 *
 * @section HIDReport HID Report Format
 * The function writes 8-byte HID keyboard reports in the standard format:
 * - Byte 0: Modifier keys (Ctrl, Alt, Shift, etc.)
 * - Byte 1: Reserved (always 0)
 * - Bytes 2-7: Up to 6 simultaneous key codes
 *
 * @note Uses WriteWithoutResponse for minimal latency
 * @note The returned lambda is safe to call even if service/characteristic become invalid
 */
auto make_report_writer(QLowEnergyService* service, QLowEnergyCharacteristic ch) {
    return [service, ch](const std::array<uint8_t, 8>& report) {
        // Validate service and characteristic before writing
        if (!service || !ch.isValid()) {
            LOG_INFO("Invalid service or characteristic, skipping HID report");
            return;
        }

        // Convert HID report to QByteArray and transmit
        QByteArray data(reinterpret_cast<const char*>(report.data()), 8);
        service->writeCharacteristic(ch, data, QLowEnergyService::WriteWithoutResponse);
    };
}

// ---------------------------------------------------------------------------
//  Main Application Entry Point
// ---------------------------------------------------------------------------

/**
 * @brief Main entry point for ninjaUSB-util application
 * @param argc Number of command-line arguments
 * @param argv Array of command-line argument strings
 * @return Exit code: 0 for success, 1 for error
 *
 * This function implements the main application logic:
 * 1. Parse and validate command-line arguments
 * 2. Configure logging system based on options
 * 3. Initialize device management for keyboard monitoring
 * 4. Discover and connect to BLE devices
 * 5. Run the main event loop for input processing
 * 6. Handle graceful shutdown and cleanup
 *
 * @section CommandLine Command-Line Options
 * - `--help`: Show usage information
 * - `--version`: Display version and build information
 * - `--verbose, -V`: Enable verbose logging with timestamps
 * - `--list-devices`: Show available BLE devices and exit
 * - `--target <addr>`: Connect to specific BLE device by address
 * - `--scan-timeout <ms>`: Set BLE device scanning timeout
 * - `--log-level <level>`: Set logging verbosity (debug, info, error)
 *
 * @section EventLoop Main Event Loop
 * The main loop performs these operations:
 * - Poll keyboard devices for input events (1ms interval)
 * - Process Qt events for BLE communication
 * - Convert input events to HID reports
 * - Transmit HID reports to connected BLE device
 * - Handle device hot-plug events (connect/disconnect)
 * - Monitor for shutdown signals
 *
 * @section ErrorHandling Error Handling
 * - Invalid arguments: Display help and exit with code 1
 * - Device initialization failure: Log error and exit
 * - BLE connection failure: Continue scanning for devices
 * - Runtime errors: Log appropriately and attempt recovery
 *
 * @section Privileges Required Privileges
 * - Root access: Required for direct access to /dev/input devices
 * - Bluetooth capability: For BLE device scanning and connection
 *
 * @note The application requires root privileges to access keyboard devices
 * @note Signal handlers are installed for graceful shutdown (SIGINT, SIGTERM)
 */
int main(int argc, char* argv[]) {
    // Parse command line arguments
    args::ArgumentParser parser(argc, argv);
    auto options = parser.parse();

    if (!options) {
        return 1;  // Parse error already reported
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
    if (g_options.verbose) {
        // Enable debug level logging in verbose mode
        logging::Logger::set_level("debug");
    } else {
        logging::Logger::set_level(g_options.log_level);
    }
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
    if (g_options.verbose) {
        LOG_DEBUG("Monitoring keyboards (hot-plug supported)...");
    }

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
    QLowEnergyController* controller = nullptr;
    QLowEnergyService* service = nullptr;
    QLowEnergyCharacteristic targetChar;
    std::function<void(const std::array<uint8_t, 8>&)> sendReport;

    // ----- Device discovery -----
    QObject::connect(&discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
                     [&](const QBluetoothDeviceInfo& info) {
                         LOG_INFO("Found device " + std::to_string(foundDevices.size()) + ": " +
                                  info.name().toStdString() + " [" +
                                  info.address().toString().toStdString() + "]");
                         foundDevices.append(info);
                     });

    QObject::connect(&discoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished, [&]() {
        if (g_options.list_devices) {
            LOG_INFO("BLE device discovery completed. Found " +
                     std::to_string(foundDevices.size()) + " devices");
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
            // Filter NinjaUSB devices
            QList<QBluetoothDeviceInfo> ninjaDevices;
            for (const auto& device : foundDevices) {
                QString deviceName = device.name();
                if (deviceName.contains("ninja", Qt::CaseInsensitive) ||
                    deviceName.contains("NinjaUSB", Qt::CaseInsensitive)) {
                    ninjaDevices.append(device);
                }
            }

            // Auto-connect logic
            if (!g_options.disable_auto_connect && ninjaDevices.size() == 1) {
                // Auto-connect to the single NinjaUSB device
                for (int i = 0; i < foundDevices.size(); ++i) {
                    if (foundDevices[i].address() == ninjaDevices[0].address()) {
                        index = i;
                        break;
                    }
                }
                LOG_INFO("Auto-connecting to NinjaUSB device: " +
                         ninjaDevices[0].name().toStdString());
                if (g_options.verbose) {
                    LOG_DEBUG("Auto-connect enabled and exactly one NinjaUSB device found");
                }
            } else if (ninjaDevices.size() > 1) {
                // Multiple NinjaUSB devices found, show them to the user
                LOG_INFO("Multiple NinjaUSB devices found:");
                for (int i = 0; i < foundDevices.size(); ++i) {
                    const auto& device = foundDevices[i];
                    QString deviceName = device.name();
                    if (deviceName.contains("ninja", Qt::CaseInsensitive) ||
                        deviceName.contains("NinjaUSB", Qt::CaseInsensitive)) {
                        LOG_INFO("  " + std::to_string(i) + ": " + device.name().toStdString() +
                                 " [" + device.address().toString().toStdString() + "]");
                    }
                }
                LOG_INFO("Choose device number: ");
                std::cin >> index;
                if (index < 0 || index >= foundDevices.size()) {
                    LOG_ERROR("Invalid device index");
                    app.quit();
                    return;
                }
            } else {
                // No NinjaUSB devices found or auto-connect disabled, show all devices
                if (g_options.disable_auto_connect && ninjaDevices.size() == 1) {
                    LOG_INFO("Auto-connect disabled. Please choose from available devices:");
                } else if (ninjaDevices.empty()) {
                    LOG_INFO("No NinjaUSB devices found. Available devices:");
                }

                for (int i = 0; i < foundDevices.size(); ++i) {
                    const auto& device = foundDevices[i];
                    LOG_INFO("  " + std::to_string(i) + ": " + device.name().toStdString() + " [" +
                             device.address().toString().toStdString() + "]");
                }
                LOG_INFO("Choose device number: ");
                std::cin >> index;
                if (index < 0 || index >= foundDevices.size()) {
                    LOG_ERROR("Invalid device index");
                    app.quit();
                    return;
                }
            }
        }

        const QBluetoothDeviceInfo& device = foundDevices[index];
        controller = QLowEnergyController::createCentral(device);

        LOG_INFO("Connecting to device: " + device.name().toStdString());

        QObject::connect(controller, &QLowEnergyController::connected, [&]() {
            LOG_INFO("Connected. Discovering services...");
            controller->discoverServices();
        });
        QObject::connect(controller, &QLowEnergyController::disconnected, [&]() {
            LOG_WARN("Disconnected from BLE device");
            g_running = false;
            app.quit();
        });

        // Handle connection errors
        QObject::connect(
            controller,
            QOverload<QLowEnergyController::Error>::of(&QLowEnergyController::errorOccurred),
            [&](QLowEnergyController::Error error) {
                QString errorString;
                switch (error) {
                    case QLowEnergyController::NoError:
                        return;  // No error, continue
                    case QLowEnergyController::UnknownError:
                        errorString = "Unknown error";
                        break;
                    case QLowEnergyController::UnknownRemoteDeviceError:
                        errorString = "Unknown remote device error";
                        break;
                    case QLowEnergyController::NetworkError:
                        errorString = "Network error";
                        break;
                    case QLowEnergyController::InvalidBluetoothAdapterError:
                        errorString = "Invalid Bluetooth adapter";
                        break;
                    case QLowEnergyController::ConnectionError:
                        errorString = "Connection error";
                        break;
                    case QLowEnergyController::AdvertisingError:
                        errorString = "Advertising error";
                        break;
                    case QLowEnergyController::RemoteHostClosedError:
                        errorString = "Remote host closed connection";
                        break;
                    case QLowEnergyController::AuthorizationError:
                        errorString = "Authorization error";
                        break;
                    default:
                        errorString = "Error code: " + QString::number(static_cast<int>(error));
                        break;
                }
                LOG_ERROR("BLE connection failed: " + errorString.toStdString());
                g_running = false;
                app.quit();
            });

        // Handle connection state changes
        QObject::connect(controller, &QLowEnergyController::stateChanged,
                         [&](QLowEnergyController::ControllerState state) {
                             if (g_options.verbose) {
                                 QString stateString;
                                 switch (state) {
                                     case QLowEnergyController::UnconnectedState:
                                         stateString = "Unconnected";
                                         break;
                                     case QLowEnergyController::ConnectingState:
                                         stateString = "Connecting";
                                         break;
                                     case QLowEnergyController::ConnectedState:
                                         stateString = "Connected";
                                         break;
                                     case QLowEnergyController::DiscoveringState:
                                         stateString = "Discovering";
                                         break;
                                     case QLowEnergyController::DiscoveredState:
                                         stateString = "Discovered";
                                         break;
                                     case QLowEnergyController::ClosingState:
                                         stateString = "Closing";
                                         break;
                                     case QLowEnergyController::AdvertisingState:
                                         stateString = "Advertising";
                                         break;
                                     default:
                                         stateString = "Unknown state: " +
                                                       QString::number(static_cast<int>(state));
                                         break;
                                 }
                                 LOG_DEBUG("BLE Controller state: " + stateString.toStdString());
                             }
                         });

        QObject::connect(controller, &QLowEnergyController::serviceDiscovered,
                         [&](const QBluetoothUuid& uuid) {
                             if (g_options.verbose) {
                                 LOG_DEBUG("Service discovered: " + uuid.toString().toStdString());
                             }
                         });

        QObject::connect(controller, &QLowEnergyController::discoveryFinished, [&]() {
            if (g_options.verbose) {
                LOG_DEBUG("Service discovery finished");
            }
            // Pick first service and search for writable char
            for (const QBluetoothUuid& uuid : controller->services()) {
                service = controller->createServiceObject(uuid);
                if (!service)
                    continue;

                QObject::connect(
                    service, &QLowEnergyService::stateChanged,
                    [&](QLowEnergyService::ServiceState s) {
                        if (s != QLowEnergyService::RemoteServiceDiscovered)
                            return;
                        for (const auto& c : service->characteristics()) {
                            if (c.properties() & (QLowEnergyCharacteristic::Write |
                                                  QLowEnergyCharacteristic::WriteNoResponse)) {
                                targetChar = c;
                                sendReport = make_report_writer(service, targetChar);
                                LOG_INFO("✔ Found writable characteristic: " +
                                         c.uuid().toString().toStdString());
                                LOG_INFO(
                                    "Ready! Start typing – Alt+Ctrl+H to quit (Ctrl+C disabled).");
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

        // Set up connection timeout (30 seconds)
        QTimer* connectionTimer = new QTimer();
        connectionTimer->setSingleShot(true);
        connectionTimer->setInterval(30000);  // 30 seconds

        QObject::connect(connectionTimer, &QTimer::timeout, [&, connectionTimer]() {
            LOG_ERROR("BLE connection timeout - failed to connect within 30 seconds");
            connectionTimer->deleteLater();
            g_running = false;
            app.quit();
        });

        // Stop timer when connected successfully
        QObject::connect(controller, &QLowEnergyController::connected, [connectionTimer]() {
            connectionTimer->stop();
            connectionTimer->deleteLater();
        });

        // Start connection timeout timer before attempting connection
        connectionTimer->start();
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
            return;  // non-blocking

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
                    LOG_DEBUG("Key event: code=" + std::to_string(ev.code) + " value=" +
                              std::to_string(ev.value) + " from " + keyboards[i].name());
                }

                // Check for exit hotkey (Alt+Ctrl+H)
                static ExitHotkeyDetector hotkey_detector(true);  // Enable logging
                if (hotkey_detector.process_key_event(ev.code, ev.value)) {
                    LOG_INFO("Exit hotkey detected (Alt+Ctrl+H) - stopping program...");
                    // Send empty report to release all keys before exit
                    if (sendReport) {
                        sendReport({0, 0, 0, 0, 0, 0, 0, 0});
                        if (g_options.verbose) {
                            LOG_DEBUG("Sent empty HID report before exit");
                        }
                    }
                    LOG_INFO("Stopping HID reports and exiting...");
                    g_running = false;
                    app.quit();
                    return;
                }

                if (g_options.verbose) {
                    LOG_DEBUG("Hotkey state: " + hotkey_detector.get_state_description());
                }

                switch (ev.value) {
                    case 1:  // key down
                    case 2:  // auto-repeat
                        if (hid::apply_key_event(kb_state, ev.code, ev.value)) {
                            auto report = kb_state.get_report();
                            sendReport(report);
                            if (g_options.verbose) {
                                LOG_DEBUG(
                                    "Sent HID report: [" + std::to_string(report[0]) + ", " +
                                    std::to_string(report[1]) + ", " + std::to_string(report[2]) +
                                    ", " + std::to_string(report[3]) + ", " +
                                    std::to_string(report[4]) + ", " + std::to_string(report[5]) +
                                    ", " + std::to_string(report[6]) + ", " +
                                    std::to_string(report[7]) + "]");
                            }
                        }
                        break;
                    case 0:  // key release
                        [[maybe_unused]] bool changed =
                            hid::apply_key_event(kb_state, ev.code, ev.value);
                        // Send all-zero release report to stop the key
                        sendReport({0, 0, 0, 0, 0, 0, 0, 0});
                        if (g_options.verbose) {
                            LOG_DEBUG("Sent key release HID report: [0, 0, 0, 0, 0, 0, 0, 0]");
                        }
                        break;
                }
            }
        }
    });
    pollTimer.start();

    int ret = app.exec();
    return ret;
}