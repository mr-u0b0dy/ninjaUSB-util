#include "hid_keycodes.hpp" // header with ASCII→HID maps + helpers
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
#include <csignal>
#include <fcntl.h>
#include <iostream>
#include <libevdev/libevdev.h>
#include <libudev.h>
#include <poll.h>
#include <string>
#include <unistd.h>
#include <vector>

// ---------------------------------------------------------------------------
//  Constants
// ---------------------------------------------------------------------------
constexpr int SCAN_TIMEOUT_MS = 10'000; // BLE scan timeout (ms)

// ---------------------------------------------------------------------------
//  Global state & signal handling
// ---------------------------------------------------------------------------
std::atomic<bool> g_running{true};

void signal_handler(int signum) {
  std::cout << "\nCaught signal " << signum << ", exiting...\n";
  g_running = false;
}

// ---------------------------------------------------------------------------
//  Device wrapper
// ---------------------------------------------------------------------------
struct KeyboardDevice {
  int fd{-1};
  libevdev *evdev{nullptr};
  std::string path; // /dev/input/eventX
};

// ---------------------------------------------------------------------------
//  Helpers to open / close keyboard devices
// ---------------------------------------------------------------------------
static bool is_keyboard(libevdev *dev) {
  return libevdev_has_event_type(dev, EV_KEY) &&
         libevdev_has_event_code(dev, EV_KEY, KEY_A);
}

KeyboardDevice open_keyboard(const std::string &devnode) {
  KeyboardDevice kbd{};
  kbd.path = devnode;

  int fd = open(devnode.c_str(), O_RDONLY | O_NONBLOCK);
  if (fd < 0)
    return kbd;

  libevdev *evdev = nullptr;
  if (libevdev_new_from_fd(fd, &evdev) < 0) {
    close(fd);
    return kbd;
  }

  if (!is_keyboard(evdev)) {
    libevdev_free(evdev);
    close(fd);
    return kbd;
  }

  kbd.fd = fd;
  kbd.evdev = evdev;
  std::cout << "[+] Added keyboard: " << devnode << " ("
            << libevdev_get_name(evdev) << ")\n";
  return kbd;
}

void close_keyboard(KeyboardDevice &kbd) {
  if (kbd.evdev)
    libevdev_free(kbd.evdev);
  if (kbd.fd >= 0)
    close(kbd.fd);
  std::cout << "[-] Removed keyboard: " << kbd.path << "\n";
  kbd.fd = -1;
  kbd.evdev = nullptr;
}

// ---------------------------------------------------------------------------
//  Discover existing keyboards at startup
// ---------------------------------------------------------------------------
std::vector<KeyboardDevice> enumerate_keyboards(struct udev *udev) {
  std::vector<KeyboardDevice> out;
  struct udev_enumerate *enu = udev_enumerate_new(udev);
  udev_enumerate_add_match_subsystem(enu, "input");
  udev_enumerate_scan_devices(enu);

  udev_list_entry *devices = udev_enumerate_get_list_entry(enu);
  udev_list_entry *entry;
  udev_list_entry_foreach(entry, devices) {
    const char *syspath = udev_list_entry_get_name(entry);
    udev_device *dev = udev_device_new_from_syspath(udev, syspath);
    const char *devnode = udev_device_get_devnode(dev);
    if (devnode && strstr(devnode, "event")) {
      KeyboardDevice k = open_keyboard(devnode);
      if (k.fd >= 0)
        out.push_back(std::move(k));
    }
    udev_device_unref(dev);
  }
  udev_enumerate_unref(enu);
  return out;
}

// ---------------------------------------------------------------------------
//  Helper – write 8‑byte keyboard reports
// ---------------------------------------------------------------------------
auto make_writer(QLowEnergyService *service, QLowEnergyCharacteristic ch) {
  return [service, ch](const std::array<uint8_t, 8> &rep) {
    if (!service || !ch.isValid())
      return;
    QByteArray data(reinterpret_cast<const char *>(rep.data()), 8);
    service->writeCharacteristic(ch, data,
                                 QLowEnergyService::WriteWithoutResponse);
  };
}

// ---------------------------------------------------------------------------
//  Main
// ---------------------------------------------------------------------------
int main(int argc, char *argv[]) {
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  // ------------------ udev setup ------------------
  struct udev *udev = udev_new();
  if (!udev) {
    std::cerr << "Failed to init udev\n";
    return 1;
  }
  std::vector<KeyboardDevice> keyboards = enumerate_keyboards(udev);
  struct udev_monitor *mon = udev_monitor_new_from_netlink(udev, "udev");
  udev_monitor_filter_add_match_subsystem_devtype(mon, "input", nullptr);
  udev_monitor_enable_receiving(mon);
  int mon_fd = udev_monitor_get_fd(mon);

  std::vector<struct pollfd> pfds;
  auto rebuild_pollfds = [&] {
    pfds.clear();
    for (auto &k : keyboards)
      pfds.push_back({k.fd, POLLIN, 0});
    pfds.push_back({mon_fd, POLLIN, 0});
  };
  rebuild_pollfds();

  hid::KeyboardState kb_state;
  std::cout << "Monitoring keyboards (hot‑plug supported)…\n";

  // ------------------ Qt setup ------------------
  QCoreApplication app(argc, argv);
  QBluetoothDeviceDiscoveryAgent discoveryAgent;
  discoveryAgent.setLowEnergyDiscoveryTimeout(SCAN_TIMEOUT_MS);

  QList<QBluetoothDeviceInfo> foundDevices;
  QLowEnergyController *controller = nullptr;
  QLowEnergyService *service = nullptr;
  QLowEnergyCharacteristic targetChar;
  std::function<void(const std::array<uint8_t, 8> &)>
      sendReport; // to be initialised later

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
                           std::cout
                               << "Service: " << uuid.toString().toStdString()
                               << "\n";
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
                          sendReport = make_writer(service, targetChar);
                          std::cout << "✔ Writable characteristic: "
                                    << c.uuid().toString().toStdString()
                                    << "\n";
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

  // ------------------ Input processing loop (runs in Qt timer)
  // ------------------
  QTimer pollTimer;
  pollTimer.setInterval(1); // 1 ms polling interval
  QObject::connect(&pollTimer, &QTimer::timeout, [&] {
    if (!g_running || !sendReport)
      return;

    if (poll(pfds.data(), pfds.size(), 0) <= 0)
      return; // non‑blocking

    // ---- Read evdev events ----
    for (size_t i = 0; i < keyboards.size(); ++i) {
      if (!(pfds[i].revents & POLLIN))
        continue;
      input_event ev{};
      while (libevdev_next_event(keyboards[i].evdev, LIBEVDEV_READ_FLAG_NORMAL,
                                 &ev) == 0) {
        if (ev.type != EV_KEY)
          continue;

        switch (ev.value) {
        case 1: // key down
        case 2: // auto‑repeat
          if (hid::apply_key_event(kb_state, ev.code, ev.value)) {
            sendReport(kb_state.report);
          }
          break;
        case 0: // key release
          hid::apply_key_event(kb_state, ev.code, ev.value);
          // Send an "all‑zero" release report so the host stops the key
          sendReport({0, 0, 0, 0, 0, 0, 0, 0});
          break;
        }
      }
    }

    // ---- Hot‑plug handling ----
    if (pfds.back().revents & POLLIN) {
      if (udev_device *dev = udev_monitor_receive_device(mon)) {
        const char *action = udev_device_get_action(dev);
        const char *devnode = udev_device_get_devnode(dev);
        if (devnode && strstr(devnode, "event")) {
          if (strcmp(action, "add") == 0) {
            KeyboardDevice k = open_keyboard(devnode);
            if (k.fd >= 0) {
              keyboards.push_back(std::move(k));
              rebuild_pollfds();
            }
          } else if (strcmp(action, "remove") == 0) {
            for (auto it = keyboards.begin(); it != keyboards.end(); ++it) {
              if (it->path == devnode) {
                close_keyboard(*it);
                keyboards.erase(it);
                rebuild_pollfds();
                break;
              }
            }
          }
        }
        udev_device_unref(dev);
      }
    }
  });
  pollTimer.start();

  int ret = app.exec();

  // ------------------ Cleanup ------------------
  for (auto &k : keyboards)
    close_keyboard(k);
  udev_monitor_unref(mon);
  udev_unref(udev);
  return ret;
}
