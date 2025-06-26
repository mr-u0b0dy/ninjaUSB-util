#include "hid_keycodes.hpp" // our header with maps + helpers
#include <atomic>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <libevdev/libevdev.h>
#include <libudev.h>
#include <poll.h>
#include <string>
#include <unistd.h>
#include <vector>

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
//  Main
// ---------------------------------------------------------------------------
int main() {
  // Install CTRL+C handler
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  struct udev *udev = udev_new();
  if (!udev) {
    std::cerr << "Failed to init udev\n";
    return 1;
  }

  // Initial enumeration
  std::vector<KeyboardDevice> keyboards = enumerate_keyboards(udev);

  // udev monitor for hotplug events
  struct udev_monitor *mon = udev_monitor_new_from_netlink(udev, "udev");
  udev_monitor_filter_add_match_subsystem_devtype(mon, "input", nullptr);
  udev_monitor_enable_receiving(mon);
  int mon_fd = udev_monitor_get_fd(mon);

  // Poll list setup
  std::vector<struct pollfd> pfds;
  auto rebuild_pollfds = [&]() {
    pfds.clear();
    // keyboard fds
    for (auto &k : keyboards) {
      pfds.push_back({k.fd, POLLIN, 0});
    }
    // udev monitor fd
    pfds.push_back({mon_fd, POLLIN, 0});
  };

  rebuild_pollfds();

  hid::KeyboardState kb_state;

  std::cout << "Monitoring keyboards (hot‑plug supported)…\n";

  while (g_running) {
    if (poll(pfds.data(), pfds.size(), -1) <= 0)
      continue;

    // Handle input from keyboards
    for (size_t i = 0; i < keyboards.size(); ++i) {
      if (pfds[i].revents & POLLIN) {
        input_event ev;
        while (libevdev_next_event(keyboards[i].evdev,
                                   LIBEVDEV_READ_FLAG_NORMAL, &ev) == 0) {
          if (ev.type != EV_KEY)
            continue;

          if (hid::apply_key_event(kb_state, ev.code, ev.value)) {
            // Print keyboard 8‑byte report
            std::cout << "Keyboard HID: [";
            for (auto b : kb_state.report)
              printf("0x%02X ", b);
            std::cout << "]\n";
          } else {
            auto rep = hid::consumer_report(ev.code, ev.value);
            if (rep[0] != 0 || rep[1] != 0) {
              printf("Consumer HID: [0x%02X 0x%02X]\n", rep[0], rep[1]);
            }
          }
        }
      }
    }

    // Handle udev hotplug events (monitor is at last index)
    if (pfds.back().revents & POLLIN) {
      udev_device *dev = udev_monitor_receive_device(mon);
      if (dev) {
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
            // find matching path
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
  }

  // Cleanup
  for (auto &k : keyboards)
    close_keyboard(k);
  udev_monitor_unref(mon);
  udev_unref(udev);
  return 0;
}
