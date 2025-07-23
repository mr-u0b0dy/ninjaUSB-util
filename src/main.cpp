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
#include <memory>  // Add for std::unique_ptr
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

#include "args.hpp"                     // Command-line argument parsing
#include "ble_hid_service_manager.hpp"  // BLE HID service management with universal support
#include "device_manager.hpp"           // Device enumeration and hot-plug support
#include "exit_hotkey_detector.hpp"     // Exit hotkey detection
#include "hid_keycodes.hpp"             // HID keyboard mappings and state management
#include "logger.hpp"                   // Logging utilities
#include "version.hpp"                  // Version information

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
    ble_hid::HIDServiceManager hid_manager;
    std::function<void(const std::array<uint8_t, 8>&)> sendKeyboardReport;
    std::function<void(const std::array<uint8_t, 2>&)> sendConsumerReport;

    // Declare keyboard manager but don't initialize yet
    std::unique_ptr<device::KeyboardManager> keyboard_manager;
    std::unique_ptr<hid::KeyboardState> kb_state;

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

            // Look for HID service first, then fallback to any writable service
            QLowEnergyService* hid_service = nullptr;

            for (const QBluetoothUuid& uuid : controller->services()) {
                if (ble_hid::is_hid_service(uuid)) {
                    hid_service = controller->createServiceObject(uuid);
                    if (hid_service) {
                        LOG_INFO("Found HID service: " + uuid.toString().toStdString());
                        break;
                    }
                }
            }

            // If no HID service found, try to use first available service with writable
            // characteristics
            if (!hid_service) {
                LOG_WARN("No standard HID service found, trying first available service");
                for (const QBluetoothUuid& uuid : controller->services()) {
                    hid_service = controller->createServiceObject(uuid);
                    if (hid_service) {
                        LOG_INFO("Using service: " + uuid.toString().toStdString());
                        break;
                    }
                }
            }

            if (!hid_service) {
                LOG_ERROR("No usable BLE service found");
                app.quit();
                return;
            }

            QObject::connect(
                hid_service, &QLowEnergyService::stateChanged,
                [&](QLowEnergyService::ServiceState s) {
                    if (s != QLowEnergyService::RemoteServiceDiscovered)
                        return;

                    // Initialize HID service manager
                    if (!hid_manager.initialize(hid_service)) {
                        LOG_ERROR("Failed to initialize HID service manager");
                        app.quit();
                        return;
                    }

                    // Now initialize keyboard management after BLE connection
                    LOG_INFO("Initializing keyboard monitoring...");
                    keyboard_manager = std::make_unique<device::KeyboardManager>();
                    if (!keyboard_manager->is_valid()) {
                        LOG_ERROR("Failed to initialize device monitoring");
                        app.quit();
                        return;
                    }

                    kb_state = std::make_unique<hid::KeyboardState>();

                    LOG_INFO("Found " + std::to_string(keyboard_manager->device_count()) +
                             " keyboard(s)");
                    if (g_options.verbose) {
                        LOG_DEBUG("Monitoring keyboards (hot-plug supported)...");
                    }

                    // Set up report writers
                    sendKeyboardReport = [&](const std::array<uint8_t, 8>& report) {
                        hid_manager.send_keyboard_report(report);
                    };

                    sendConsumerReport = [&](const std::array<uint8_t, 2>& report) {
                        hid_manager.send_consumer_control_report(report);
                    };

                    LOG_INFO("✔ HID Service Manager initialized successfully");
                    LOG_INFO("Ready! Start typing – Alt+Ctrl+H to quit (Ctrl+C disabled).");

                    // Log available report types
                    auto available_types = hid_manager.get_available_report_types();
                    for (auto report_type : available_types) {
                        LOG_DEBUG("Available: " +
                                  std::string(ble_hid::report_type_to_string(report_type)));
                    }

                    // Start input processing loop only after successful connection
                    QTimer* pollTimer = new QTimer();
                    pollTimer->setInterval(g_options.poll_interval);
                    QObject::connect(pollTimer, &QTimer::timeout, [&] {
                        if (!g_running || !hid_manager.is_ready() || !keyboard_manager || !kb_state)
                            return;

                        // Update device list (handle hot-plug events)
                        if (keyboard_manager->update_devices() && g_options.verbose) {
                            LOG_DEBUG("Device list updated");
                        }

                        // Get poll file descriptors
                        std::vector<int> fds = keyboard_manager->get_poll_fds();
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
                        const auto& keyboards = keyboard_manager->keyboards();
                        for (size_t i = 0; i < keyboards.size(); ++i) {
                            if (i >= pfds.size() || !(pfds[i].revents & POLLIN))
                                continue;

                            input_event ev{};
                            while (libevdev_next_event(keyboards[i].evdev(),
                                                       LIBEVDEV_READ_FLAG_NORMAL, &ev) == 0) {
                                if (ev.type != EV_KEY)
                                    continue;

                                if (g_options.verbose) {
                                    LOG_DEBUG("Key event: code=" + std::to_string(ev.code) +
                                              " value=" + std::to_string(ev.value) + " from " +
                                              keyboards[i].name());
                                }

                                // Check for exit hotkey (Alt+Ctrl+H)
                                static ExitHotkeyDetector hotkey_detector(true);  // Enable logging
                                if (hotkey_detector.process_key_event(ev.code, ev.value)) {
                                    LOG_INFO(
                                        "Exit hotkey detected (Alt+Ctrl+H) - stopping program...");
                                    // Send empty reports to release all keys before exit
                                    if (hid_manager.is_ready()) {
                                        hid_manager.send_keyboard_report({0, 0, 0, 0, 0, 0, 0, 0});
                                        hid_manager.send_consumer_control_report({0, 0});
                                        if (g_options.verbose) {
                                            LOG_DEBUG("Sent empty HID reports before exit");
                                        }
                                    }
                                    LOG_INFO("Stopping HID reports and exiting...");
                                    g_running = false;
                                    app.quit();
                                    return;
                                }

                                if (g_options.verbose) {
                                    LOG_DEBUG("Hotkey state: " +
                                              hotkey_detector.get_state_description());
                                }

                                switch (ev.value) {
                                    case 1:  // key down
                                    case 2:  // auto-repeat
                                        // Check if it's a consumer control key first
                                        if (auto consumer_usage =
                                                hid::get_consumer_usage(ev.code)) {
                                            // Handle consumer control keys (media keys)
                                            auto consumer_report =
                                                hid::make_consumer_report(ev.code, ev.value);
                                            if (hid_manager.supports_report_type(
                                                    ble_hid::HIDReportType::CONSUMER_CONTROL)) {
                                                hid_manager.send_consumer_control_report(
                                                    consumer_report);
                                                if (g_options.verbose) {
                                                    LOG_DEBUG(
                                                        "Sent Consumer Control report: [" +
                                                        std::to_string(consumer_report[0]) + ", " +
                                                        std::to_string(consumer_report[1]) + "]");
                                                }
                                            } else {
                                                LOG_DEBUG("Consumer control not supported, "
                                                          "skipping media key");
                                            }
                                        } else if (hid::apply_key_event(*kb_state, ev.code,
                                                                        ev.value)) {
                                            // Handle regular keyboard keys
                                            auto report = kb_state->get_report();
                                            if (hid_manager.supports_report_type(
                                                    ble_hid::HIDReportType::KEYBOARD_INPUT)) {
                                                hid_manager.send_keyboard_report(report);
                                                if (g_options.verbose) {
                                                    LOG_DEBUG("Sent Keyboard report: [" +
                                                              std::to_string(report[0]) + ", " +
                                                              std::to_string(report[1]) + ", " +
                                                              std::to_string(report[2]) + ", " +
                                                              std::to_string(report[3]) + ", " +
                                                              std::to_string(report[4]) + ", " +
                                                              std::to_string(report[5]) + ", " +
                                                              std::to_string(report[6]) + ", " +
                                                              std::to_string(report[7]) + "]");
                                                }
                                            } else {
                                                LOG_DEBUG("Keyboard input not supported");
                                            }
                                        }
                                        break;
                                    case 0:  // key release
                                        // Handle key release for consumer control keys
                                        if (auto consumer_usage =
                                                hid::get_consumer_usage(ev.code)) {
                                            // Send consumer control release (all zeros)
                                            if (hid_manager.supports_report_type(
                                                    ble_hid::HIDReportType::CONSUMER_CONTROL)) {
                                                hid_manager.send_consumer_control_report({0, 0});
                                                if (g_options.verbose) {
                                                    LOG_DEBUG(
                                                        "Sent Consumer Control release: [0, 0]");
                                                }
                                            }
                                        } else {
                                            // Handle regular keyboard key release
                                            [[maybe_unused]] bool changed =
                                                hid::apply_key_event(*kb_state, ev.code, ev.value);
                                            if (hid_manager.supports_report_type(
                                                    ble_hid::HIDReportType::KEYBOARD_INPUT)) {
                                                auto report = kb_state->get_report();
                                                hid_manager.send_keyboard_report(report);
                                                if (g_options.verbose) {
                                                    LOG_DEBUG("Sent Keyboard release report: [" +
                                                              std::to_string(report[0]) + ", " +
                                                              std::to_string(report[1]) + ", " +
                                                              std::to_string(report[2]) + ", " +
                                                              std::to_string(report[3]) + ", " +
                                                              std::to_string(report[4]) + ", " +
                                                              std::to_string(report[5]) + ", " +
                                                              std::to_string(report[6]) + ", " +
                                                              std::to_string(report[7]) + "]");
                                                }
                                            }
                                        }
                                        break;
                                }
                            }
                        }
                    });
                    pollTimer->start();
                });
            hid_service->discoverDetails();
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

    int ret = app.exec();
    return ret;
}