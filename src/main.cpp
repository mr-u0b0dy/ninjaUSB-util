#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <libevdev/libevdev.h>
#include <libudev.h>
#include <poll.h>
#include <unistd.h>
#include <vector>

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

    // Check if it's a keyboard
    struct libevdev *evdev = nullptr;
    int fd = open(devnode, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
      udev_device_unref(dev);
      continue;
    }

    if (libevdev_new_from_fd(fd, &evdev) < 0) {
      close(fd);
      udev_device_unref(dev);
      continue;
    }

    if (libevdev_has_event_type(evdev, EV_KEY) &&
        libevdev_has_event_code(evdev, EV_KEY, KEY_A)) {
      std::cout << "Using: " << devnode << " (" << libevdev_get_name(evdev)
                << ")\n";
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

  std::cout << "Monitoring key events...\n";

  while (true) {
    if (poll(fds.data(), fds.size(), -1) > 0) {
      for (size_t i = 0; i < keyboards.size(); ++i) {
        if (fds[i].revents & POLLIN) {
          struct input_event ev;
          while (libevdev_next_event(keyboards[i].dev,
                                     LIBEVDEV_READ_FLAG_NORMAL, &ev) == 0) {
            if (ev.type == EV_KEY) {
              std::cout << "Key " << ev.code
                        << (ev.value == 1   ? " DOWN\n"
                            : ev.value == 0 ? " UP\n"
                                            : " HOLD\n");
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
