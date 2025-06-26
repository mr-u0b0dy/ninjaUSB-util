#include "hid_keycodes.hpp"
#include <array>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <libevdev/libevdev.h>
#include <libudev.h>
#include <poll.h>
#include <set>
#include <unistd.h>
#include <vector>

std::set<uint8_t> pressedKeys;
uint8_t modifiers = 0;

void print_hid_report() {
  std::array<uint8_t, 8> report = {0};
  report[0] = modifiers;
  size_t idx = 2;

  for (uint8_t key : pressedKeys) {
    if (key >= 0xE0 && key <= 0xE7)
      continue; // Skip modifiers here
    if (idx < 8)
      report[idx++] = key;
  }

  std::cout << "HID Report: [";
  for (uint8_t b : report) {
    printf("0x%02X ", b);
  }
  std::cout << "]\n";
}

void print_consumer_report(uint16_t usage_id) {
  // This is a 2-byte report: little endian HID usage ID from consumer page
  // (0x0C)
  std::array<uint8_t, 2> report;
  report[0] = usage_id & 0xFF;
  report[1] = (usage_id >> 8) & 0xFF;

  std::cout << "Consumer HID Report: [";
  for (uint8_t b : report)
    printf("0x%02X ", b);
  std::cout << "]  (Usage ID: 0x" << std::hex << usage_id << ")\n";
}
void handle_key_event(int code, int value) {
  // First check if it's a standard keyboard key
  auto it = linuxToHID.find(code);
  if (it != linuxToHID.end()) {
    uint8_t hid_code = it->second;
    if (hid_code >= 0xE0 && hid_code <= 0xE7) {
      uint8_t bit = 1 << (hid_code - 0xE0);
      if (value == 1)
        modifiers |= bit;
      else if (value == 0)
        modifiers &= ~bit;
    } else {
      if (value == 1)
        pressedKeys.insert(hid_code);
      else if (value == 0)
        pressedKeys.erase(hid_code);
    }
    print_hid_report();
    return;
  }

  // If it's a consumer/media key
  auto it2 = linuxToConsumerHID.find(code);
  if (it2 != linuxToConsumerHID.end()) {
    if (value == 1) {
      print_consumer_report(it2->second);
    } else {
      print_consumer_report(0); // Key released = send empty report
    }
    return;
  }

  // Otherwise: unknown or unsupported key
}

struct KeyboardDevice {
  int fd;
  libevdev *dev;
};

std::vector<KeyboardDevice> find_all_keyboards() {
  std::vector<KeyboardDevice> keyboards;
  struct udev *udev = udev_new();
  struct udev_enumerate *enumerate = udev_enumerate_new(udev);
  udev_enumerate_add_match_subsystem(enumerate, "input");
  udev_enumerate_scan_devices(enumerate);

  struct udev_list_entry *devices = udev_enumerate_get_list_entry(enumerate);
  struct udev_list_entry *entry;

  udev_list_entry_foreach(entry, devices) {
    const char *path = udev_list_entry_get_name(entry);
    struct udev_device *dev = udev_device_new_from_syspath(udev, path);
    const char *devnode = udev_device_get_devnode(dev);
    if (!devnode || !strstr(devnode, "event")) {
      udev_device_unref(dev);
      continue;
    }

    int fd = open(devnode, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
      udev_device_unref(dev);
      continue;
    }

    libevdev *evdev = nullptr;
    if (libevdev_new_from_fd(fd, &evdev) < 0) {
      close(fd);
      udev_device_unref(dev);
      continue;
    }

    if (libevdev_has_event_type(evdev, EV_KEY) &&
        libevdev_has_event_code(evdev, EV_KEY, KEY_A)) {
      std::cout << "Using device: " << devnode << " ("
                << libevdev_get_name(evdev) << ")\n";
      keyboards.push_back({fd, evdev});
    } else {
      libevdev_free(evdev);
      close(fd);
    }

    udev_device_unref(dev);
  }

  udev_enumerate_unref(enumerate);
  udev_unref(udev);
  return keyboards;
}

int main() {
  auto keyboards = find_all_keyboards();
  if (keyboards.empty()) {
    std::cerr << "No keyboards found.\n";
    return 1;
  }

  std::vector<struct pollfd> fds;
  for (const auto &kbd : keyboards) {
    fds.push_back({kbd.fd, POLLIN, 0});
  }

  std::cout << "Monitoring keyboard input and printing HID reports...\n";

  while (true) {
    if (poll(fds.data(), fds.size(), -1) > 0) {
      for (size_t i = 0; i < keyboards.size(); ++i) {
        if (fds[i].revents & POLLIN) {
          struct input_event ev;
          while (libevdev_next_event(keyboards[i].dev,
                                     LIBEVDEV_READ_FLAG_NORMAL, &ev) == 0) {
            if (ev.type == EV_KEY) {
              handle_key_event(ev.code, ev.value);
            }
          }
        }
      }
    }
  }

  for (auto &kbd : keyboards) {
    libevdev_free(kbd.dev);
    close(kbd.fd);
  }
}
