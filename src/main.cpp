#include <array>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <libevdev/libevdev.h>
#include <libudev.h>
#include <map>
#include <poll.h>
#include <set>
#include <unistd.h>
#include <vector>

// Linux key code to HID usage ID (partial)
std::map<int, uint8_t> linuxToHID = {
    {KEY_A, 0x04},          {KEY_B, 0x05},        {KEY_C, 0x06},
    {KEY_D, 0x07},          {KEY_E, 0x08},        {KEY_F, 0x09},
    {KEY_G, 0x0A},          {KEY_H, 0x0B},        {KEY_I, 0x0C},
    {KEY_J, 0x0D},          {KEY_K, 0x0E},        {KEY_L, 0x0F},
    {KEY_M, 0x10},          {KEY_N, 0x11},        {KEY_O, 0x12},
    {KEY_P, 0x13},          {KEY_Q, 0x14},        {KEY_R, 0x15},
    {KEY_S, 0x16},          {KEY_T, 0x17},        {KEY_U, 0x18},
    {KEY_V, 0x19},          {KEY_W, 0x1A},        {KEY_X, 0x1B},
    {KEY_Y, 0x1C},          {KEY_Z, 0x1D},        {KEY_1, 0x1E},
    {KEY_2, 0x1F},          {KEY_3, 0x20},        {KEY_4, 0x21},
    {KEY_5, 0x22},          {KEY_6, 0x23},        {KEY_7, 0x24},
    {KEY_8, 0x25},          {KEY_9, 0x26},        {KEY_0, 0x27},
    {KEY_ENTER, 0x28},      {KEY_ESC, 0x29},      {KEY_BACKSPACE, 0x2A},
    {KEY_TAB, 0x2B},        {KEY_SPACE, 0x2C},    {KEY_LEFTSHIFT, 0xE1},
    {KEY_RIGHTSHIFT, 0xE5}, {KEY_LEFTCTRL, 0xE0}, {KEY_RIGHTCTRL, 0xE4},
    {KEY_LEFTALT, 0xE2},    {KEY_RIGHTALT, 0xE6},
};

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

void handle_key_event(int code, int value) {
  if (linuxToHID.find(code) == linuxToHID.end())
    return;
  uint8_t hid_code = linuxToHID[code];

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
