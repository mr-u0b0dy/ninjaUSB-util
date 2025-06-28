// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2025 Dharun A P

#pragma once

#include <string>
#include <vector>
#include <functional>

// Forward declarations for system headers
struct udev;
struct udev_monitor;
struct libevdev;

namespace device {

/**
 * @brief RAII wrapper for keyboard input device
 */
class KeyboardDevice {
private:
    int fd_{-1};
    libevdev* evdev_{nullptr};
    std::string path_;
    std::string name_;

public:
    explicit KeyboardDevice(const std::string& device_path);
    ~KeyboardDevice();

    // Non-copyable but movable
    KeyboardDevice(const KeyboardDevice&) = delete;
    KeyboardDevice& operator=(const KeyboardDevice&) = delete;
    KeyboardDevice(KeyboardDevice&& other) noexcept;
    KeyboardDevice& operator=(KeyboardDevice&& other) noexcept;

    [[nodiscard]] bool is_valid() const noexcept { return fd_ >= 0 && evdev_ != nullptr; }
    [[nodiscard]] int fd() const noexcept { return fd_; }
    [[nodiscard]] libevdev* evdev() const noexcept { return evdev_; }
    [[nodiscard]] const std::string& path() const noexcept { return path_; }
    [[nodiscard]] const std::string& name() const noexcept { return name_; }

private:
    void cleanup() noexcept;
    static bool is_keyboard_device(libevdev* dev) noexcept;
};

/**
 * @brief RAII wrapper for udev and device monitoring
 */
class DeviceMonitor {
private:
    udev* udev_{nullptr};
    udev_monitor* monitor_{nullptr};
    int monitor_fd_{-1};

public:
    DeviceMonitor();
    ~DeviceMonitor();

    // Non-copyable, non-movable for simplicity
    DeviceMonitor(const DeviceMonitor&) = delete;
    DeviceMonitor& operator=(const DeviceMonitor&) = delete;
    DeviceMonitor(DeviceMonitor&&) = delete;
    DeviceMonitor& operator=(DeviceMonitor&&) = delete;

    [[nodiscard]] bool is_valid() const noexcept { return udev_ != nullptr && monitor_ != nullptr; }
    [[nodiscard]] int monitor_fd() const noexcept { return monitor_fd_; }

    /**
     * @brief Enumerate existing keyboard devices
     * @return Vector of valid keyboard devices
     */
    [[nodiscard]] std::vector<KeyboardDevice> enumerate_keyboards() const;

    /**
     * @brief Check for device add/remove events
     * @param on_add Callback for device addition (device path)
     * @param on_remove Callback for device removal (device path)
     * @return True if events were processed
     */
    bool process_events(
        const std::function<void(const std::string&)>& on_add,
        const std::function<void(const std::string&)>& on_remove
    ) const;

private:
    void cleanup() noexcept;
};

/**
 * @brief Manages collection of keyboard devices with hot-plug support
 */
class KeyboardManager {
private:
    std::vector<KeyboardDevice> keyboards_;
    DeviceMonitor monitor_;

public:
    KeyboardManager();

    [[nodiscard]] bool is_valid() const noexcept { return monitor_.is_valid(); }
    [[nodiscard]] const std::vector<KeyboardDevice>& keyboards() const noexcept { return keyboards_; }
    [[nodiscard]] std::size_t device_count() const noexcept { return keyboards_.size(); }
    [[nodiscard]] int monitor_fd() const noexcept { return monitor_.monitor_fd(); }

    /**
     * @brief Process hot-plug events and update device list
     * @return True if the device list was modified
     */
    bool update_devices();

    /**
     * @brief Get file descriptors for polling
     * @return Vector of file descriptors (keyboards + monitor)
     */
    [[nodiscard]] std::vector<int> get_poll_fds() const;

private:
    void add_device(const std::string& device_path);
    void remove_device(const std::string& device_path);
};

} // namespace device
