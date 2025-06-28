// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2025 Dharun A P

#include "device_manager.hpp"
#include "logger.hpp"
#include <algorithm>
#include <cstring>
#include <vector>     // Add missing vector header
#include <functional> // Add missing functional header
#include <cerrno>     // Add for errno support
#include <fcntl.h>
#include <unistd.h>
#include <libevdev/libevdev.h>
#include <libudev.h>

namespace device {

// Constants for better code maintainability
namespace {
    constexpr int INVALID_FD = -1;
    constexpr const char* EVENT_DEVICE_PREFIX = "event";
    constexpr const char* INPUT_SUBSYSTEM = "input";
    constexpr const char* UDEV_SOURCE = "udev";
    constexpr const char* ACTION_ADD = "add";
    constexpr const char* ACTION_REMOVE = "remove";
}

// Add logging utility for consistent error reporting
namespace {
    void log_error(const std::string& message) {
        LOG_ERROR(message);
    }
    
    void log_info(const std::string& message) {
        LOG_INFO(message);
    }
    
    void log_debug(const std::string& message) {
        LOG_DEBUG(message);
    }
}

//-----------------------------------------------------------------------------
// KeyboardDevice implementation
//-----------------------------------------------------------------------------

KeyboardDevice::KeyboardDevice(const std::string& device_path) 
    : path_(device_path), fd_(INVALID_FD) {
    
    // Input validation
    if (device_path.empty()) {
        log_error("Empty device path provided");
        return;
    }
    
    fd_ = open(device_path.c_str(), O_RDONLY | O_NONBLOCK);
    if (fd_ < 0) {
        log_error("Failed to open device: " + device_path + " (" + std::strerror(errno) + ")");
        return;
    }

    if (libevdev_new_from_fd(fd_, &evdev_) < 0) {
        close(fd_);
        fd_ = INVALID_FD;
        return;
    }

    if (!is_keyboard_device(evdev_)) {
        cleanup();
        return;
    }

    // Cache device name
    const char* dev_name = libevdev_get_name(evdev_);
    name_ = dev_name ? dev_name : "Unknown Device";
    
    log_info("Added keyboard: " + path_ + " (" + name_ + ")");
}

KeyboardDevice::~KeyboardDevice() {
    cleanup();
}

KeyboardDevice::KeyboardDevice(KeyboardDevice&& other) noexcept
    : fd_(other.fd_), evdev_(other.evdev_), path_(std::move(other.path_)), name_(std::move(other.name_)) {
    other.fd_ = INVALID_FD;
    other.evdev_ = nullptr;
}

KeyboardDevice& KeyboardDevice::operator=(KeyboardDevice&& other) noexcept {
    if (this != &other) {
        cleanup();
        
        fd_ = other.fd_;
        evdev_ = other.evdev_;
        path_ = std::move(other.path_);
        name_ = std::move(other.name_);
        
        other.fd_ = INVALID_FD;
        other.evdev_ = nullptr;
    }
    return *this;
}

void KeyboardDevice::cleanup() noexcept {
    if (evdev_) {
        libevdev_free(evdev_);
        evdev_ = nullptr;
    }
    if (fd_ >= 0) {
        close(fd_);
        fd_ = INVALID_FD;
        log_info("Removed keyboard: " + path_);
    }
}

bool KeyboardDevice::is_keyboard_device(libevdev* dev) noexcept {
    return dev && 
           libevdev_has_event_type(dev, EV_KEY) &&
           libevdev_has_event_code(dev, EV_KEY, KEY_A);
}

//-----------------------------------------------------------------------------
// DeviceMonitor implementation
//-----------------------------------------------------------------------------

DeviceMonitor::DeviceMonitor() {
    udev_ = udev_new();
    if (!udev_) {
        log_error("Failed to initialize udev");
        return;
    }

    monitor_ = udev_monitor_new_from_netlink(udev_, UDEV_SOURCE);
    if (!monitor_) {
        log_error("Failed to create udev monitor");
        cleanup();
        return;
    }

    if (udev_monitor_filter_add_match_subsystem_devtype(monitor_, INPUT_SUBSYSTEM, nullptr) < 0) {
        log_error("Failed to add udev filter");
        cleanup();
        return;
    }

    if (udev_monitor_enable_receiving(monitor_) < 0) {
        log_error("Failed to enable udev monitoring");
        cleanup();
        return;
    }

    monitor_fd_ = udev_monitor_get_fd(monitor_);
    if (monitor_fd_ < 0) {
        log_error("Failed to get udev monitor fd");
        cleanup();
        return;
    }
}

DeviceMonitor::~DeviceMonitor() {
    cleanup();
}

void DeviceMonitor::cleanup() noexcept {
    if (monitor_) {
        udev_monitor_unref(monitor_);
        monitor_ = nullptr;
    }
    if (udev_) {
        udev_unref(udev_);
        udev_ = nullptr;
    }
    monitor_fd_ = INVALID_FD;
}

std::vector<KeyboardDevice> DeviceMonitor::enumerate_keyboards() const {
    std::vector<KeyboardDevice> keyboards;
    
    if (!udev_) {
        return keyboards;
    }

    udev_enumerate* enumerate = udev_enumerate_new(udev_);
    if (!enumerate) {
        return keyboards;
    }

    udev_enumerate_add_match_subsystem(enumerate, INPUT_SUBSYSTEM);
    udev_enumerate_scan_devices(enumerate);

    udev_list_entry* devices = udev_enumerate_get_list_entry(enumerate);
    udev_list_entry* entry = nullptr;
    
    udev_list_entry_foreach(entry, devices) {
        const char* syspath = udev_list_entry_get_name(entry);
        udev_device* dev = udev_device_new_from_syspath(udev_, syspath);
        
        if (dev) {
            const char* devnode = udev_device_get_devnode(dev);
            if (devnode && std::strstr(devnode, EVENT_DEVICE_PREFIX)) {
                KeyboardDevice kbd(devnode);
                if (kbd.is_valid()) {
                    keyboards.emplace_back(std::move(kbd));
                }
            }
            udev_device_unref(dev);
        }
    }

    udev_enumerate_unref(enumerate);
    return keyboards;
}

bool DeviceMonitor::process_events(
    const std::function<void(const std::string&)>& on_add,
    const std::function<void(const std::string&)>& on_remove
) const {
    if (!monitor_) {
        return false;
    }

    bool events_processed = false;
    udev_device* dev = nullptr;
    
    while ((dev = udev_monitor_receive_device(monitor_)) != nullptr) {
        events_processed = true;
        
        const char* action = udev_device_get_action(dev);
        const char* devnode = udev_device_get_devnode(dev);
        
        if (devnode && std::strstr(devnode, EVENT_DEVICE_PREFIX) && action) {
            if (std::strcmp(action, ACTION_ADD) == 0 && on_add) {
                on_add(devnode);
            } else if (std::strcmp(action, ACTION_REMOVE) == 0 && on_remove) {
                on_remove(devnode);
            }
        }
        
        udev_device_unref(dev);
    }
    
    return events_processed;
}

//-----------------------------------------------------------------------------
// KeyboardManager implementation
//-----------------------------------------------------------------------------

KeyboardManager::KeyboardManager() {
    if (monitor_.is_valid()) {
        keyboards_ = monitor_.enumerate_keyboards();
        log_info("Found " + std::to_string(keyboards_.size()) + " keyboard(s) at startup");
    }
}

bool KeyboardManager::update_devices() {
    if (!monitor_.is_valid()) {
        return false;
    }

    bool devices_changed = false;
    
    auto on_add = [this, &devices_changed](const std::string& path) {
        add_device(path);
        devices_changed = true;
    };
    
    auto on_remove = [this, &devices_changed](const std::string& path) {
        remove_device(path);
        devices_changed = true;
    };
    
    monitor_.process_events(on_add, on_remove);
    return devices_changed;
}

std::vector<int> KeyboardManager::get_poll_fds() const {
    std::vector<int> fds;
    fds.reserve(keyboards_.size() + 1);
    
    for (const auto& kbd : keyboards_) {
        if (kbd.is_valid()) {
            fds.push_back(kbd.fd());
        }
    }
    
    if (monitor_.is_valid()) {
        fds.push_back(monitor_.monitor_fd());
    }
    
    return fds;
}

void KeyboardManager::add_device(const std::string& device_path) {
    // Check if device already exists
    auto it = std::find_if(keyboards_.begin(), keyboards_.end(),
        [&device_path](const KeyboardDevice& kbd) {
            return kbd.path() == device_path;
        });
    
    if (it != keyboards_.end()) {
        return; // Already exists
    }
    
    KeyboardDevice kbd(device_path);
    if (kbd.is_valid()) {
        keyboards_.emplace_back(std::move(kbd));
    }
}

void KeyboardManager::remove_device(const std::string& device_path) {
    auto it = std::find_if(keyboards_.begin(), keyboards_.end(),
        [&device_path](const KeyboardDevice& kbd) {
            return kbd.path() == device_path;
        });
    
    if (it != keyboards_.end()) {
        keyboards_.erase(it);
    }
}

} // namespace device
