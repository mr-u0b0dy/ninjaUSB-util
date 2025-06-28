// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2025 Dharun A P

#include "hid_keycodes.hpp"    // HID keyboard mappings and state management
#include "device_manager.hpp"  // Device enumeration and hot-plug support
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
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
//  Constants & Global state
// ---------------------------------------------------------------------------
constexpr int SCAN_TIMEOUT_MS = 10'000; // BLE scan timeout (ms)
constexpr int POLL_INTERVAL_MS = 1;     // Input polling interval (ms)

std::atomic<bool> g_running{true};

void signal_handler(int signum) {
    std::cout << "\nCaught signal " << signum << ", exiting...\n";
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
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // ------------------ Initialize device management ------------------
    device::KeyboardManager keyboard_manager;
    if (!keyboard_manager.is_valid()) {
        std::cerr << "Failed to initialize device monitoring\n";
        return 1;
    }

    std::cout << "Found " << keyboard_manager.device_count() << " keyboard(s)\n";
    std::cout << "Monitoring keyboards (hot-plug supported)...\n";

    hid::KeyboardState kb_state;

    // ------------------ Qt setup ------------------
    QCoreApplication app(argc, argv);
    QBluetoothDeviceDiscoveryAgent discoveryAgent;
    discoveryAgent.setLowEnergyDiscoveryTimeout(SCAN_TIMEOUT_MS);

    QList<QBluetoothDeviceInfo> foundDevices;
    QLowEnergyController *controller = nullptr;
    QLowEnergyService *service = nullptr;
    QLowEnergyCharacteristic targetChar;
    std::function<void(const std::array<uint8_t, 8> &)> sendReport;

    // ----- Device discovery -----
    QObject::connect(
        &discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
        [&](const QBluetoothDeviceInfo &info) {
            std::cout << foundDevices.size() << ": " << info.name().toStdString()
                      << " [" << info.address().toString().toStdString() << "]\n";
            foundDevices.append(info);
        });

    QObject::connect(
        &discoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished, [&]() {
            if (foundDevices.empty()) {
                std::cerr << "No BLE devices found – exiting.\n";
                app.quit();
                return;
            }
            std::cout << "Discovery complete. Choose device number: ";
            int index{0};
            std::cin >> index;
            if (index < 0 || index >= foundDevices.size()) {
                std::cerr << "Invalid index\n";
                app.quit();
                return;
            }

            const QBluetoothDeviceInfo &device = foundDevices[index];
            controller = QLowEnergyController::createCentral(device);

            QObject::connect(controller, &QLowEnergyController::connected, [&]() {
                std::cout << "Connected. Discovering services…\n";
                controller->discoverServices();
            });
            QObject::connect(controller, &QLowEnergyController::disconnected,
                             [&]() {
                                 std::cerr << "Disconnected.\n";
                                 g_running = false;
                                 app.quit();
                             });

            QObject::connect(controller, &QLowEnergyController::serviceDiscovered,
                             [&](const QBluetoothUuid &uuid) {
                                 std::cout << "Service: " << uuid.toString().toStdString() << "\n";
                             });

            QObject::connect(
                controller, &QLowEnergyController::discoveryFinished, [&]() {
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
                                        std::cout << "✔ Writable characteristic: "
                                                  << c.uuid().toString().toStdString() << "\n";
                                        std::cout << "Start typing – Ctrl+C to quit.\n";
                                    }
                                }
                            });
                        service->discoverDetails();
                        if (targetChar.isValid())
                            break;
                    }
                    if (!targetChar.isValid()) {
                        std::cerr << "No writable characteristic found.\n";
                        app.quit();
                    }
                });
            controller->connectToDevice();
        });

    discoveryAgent.start();

    // ------------------ Input processing loop ------------------
    QTimer pollTimer;
    pollTimer.setInterval(POLL_INTERVAL_MS);
    QObject::connect(&pollTimer, &QTimer::timeout, [&] {
        if (!g_running || !sendReport)
            return;

        // Update device list (handle hot-plug events)
        keyboard_manager.update_devices();

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