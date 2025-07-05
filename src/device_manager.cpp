/**
 * @file device_manager.cpp
 * @brief Implementation of device management functionality
 * @author Dharun A P
 * @license SPDX-License-Identifier: Apache-2.0
 * @copyright SPDX-FileCopyrightText: 2025 Dharun A P
 */

#include "device_manager.hpp"

#include <algorithm>
#include <cerrno>  // Add for errno support
#include <cstring>
#include <fcntl.h>
#include <functional>  // Add missing functional header
#include <libudev.h>
#include <unistd.h>
#include <vector>  // Add missing vector header

#include <libevdev/libevdev.h>

#include "logger.hpp"

namespace device {

/**
 * @brief Anonymous namespace containing implementation constants and utilities
 *
 * This namespace provides:
 * - File descriptor and device type constants for consistent error handling
 * - String constants for udev subsystem and action filtering
 * - Logging wrapper functions for consistent message formatting
 *
 * These utilities help maintain clean separation between interface and
 * implementation details while providing centralized configuration.
 */
namespace {
constexpr int INVALID_FD = -1;                        //!< Invalid file descriptor constant
constexpr const char* EVENT_DEVICE_PREFIX = "event";  //!< Prefix for input event devices
constexpr const char* INPUT_SUBSYSTEM = "input";      //!< udev subsystem name for input devices
constexpr const char* UDEV_SOURCE = "udev";           //!< udev event source identifier
constexpr const char* ACTION_ADD = "add";             //!< udev action for device addition
constexpr const char* ACTION_REMOVE = "remove";       //!< udev action for device removal
}  // namespace

/**
 * @brief Anonymous namespace containing logging utility functions
 *
 * Provides wrapper functions around the global logging system for consistent
 * error reporting and debugging within the device management module.
 * These wrappers allow for easy modification of logging behavior without
 * changing call sites throughout the implementation.
 */
namespace {
/**
 * @brief Log an error message with device management context
 * @param message The error message to log
 */
void log_error(const std::string& message) {
    LOG_ERROR(message);
}

/**
 * @brief Log an informational message with device management context
 * @param message The informational message to log
 */
void log_info(const std::string& message) {
    LOG_INFO(message);
}

/**
 * @brief Log a debug message with device management context
 * @param message The debug message to log
 */
void log_debug(const std::string& message) {
    LOG_DEBUG(message);
}
}  // namespace

//-----------------------------------------------------------------------------
// KeyboardDevice implementation
//-----------------------------------------------------------------------------

/**
 * @brief Construct KeyboardDevice from device path with validation and initialization
 * @param device_path Path to the input device (e.g., "/dev/input/event0")
 *
 * The constructor performs several initialization steps:
 * 1. Input validation of the device path
 * 2. Opens the device file with non-blocking read access
 * 3. Initializes libevdev context for event processing
 * 4. Validates that the device supports keyboard input
 * 5. Caches the human-readable device name
 *
 * If any step fails, the device is marked as invalid but remains safe to use.
 * Error details are logged for debugging purposes.
 *
 * @note The device file is opened with O_NONBLOCK to prevent blocking on read operations
 * @note Device validation ensures only actual keyboards are accepted
 * @note Failed initialization leaves the object in a safe, invalid state
 */
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

    log_debug("Added keyboard: " + path_ + " (" + name_ + ")");
}

/**
 * @brief Destructor - automatically clean up all allocated resources
 *
 * Ensures proper cleanup of file descriptors and libevdev contexts
 * to prevent resource leaks. Safe to call on invalid devices.
 */
KeyboardDevice::~KeyboardDevice() {
    cleanup();
}

/**
 * @brief Move constructor for efficient transfer of device ownership
 * @param other Device to move from (left in valid but empty state)
 *
 * Transfers all resources from the source device and leaves it in a
 * valid but empty state. This enables efficient storage in containers
 * without resource duplication.
 *
 * @note The moved-from object remains valid but becomes non-functional
 * @note No resource copying occurs - only ownership transfer
 */
KeyboardDevice::KeyboardDevice(KeyboardDevice&& other) noexcept
    : fd_(other.fd_), evdev_(other.evdev_), path_(std::move(other.path_)),
      name_(std::move(other.name_)) {
    other.fd_ = INVALID_FD;
    other.evdev_ = nullptr;
}

/**
 * @brief Move assignment operator for efficient device transfer
 * @param other Device to move from
 * @return Reference to this device after assignment
 *
 * Performs self-assignment protection, cleans up current resources,
 * and transfers ownership from the source device. The source device
 * is left in a valid but empty state.
 *
 * @note Self-assignment is safely handled and results in no operation
 * @note Current resources are properly cleaned up before assignment
 */
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

/**
 * @brief Internal cleanup function for safe resource deallocation
 *
 * Safely releases all allocated resources including libevdev context and
 * file descriptor. This function is safe to call multiple times and with
 * partially initialized objects.
 *
 * @note Function is noexcept to ensure it can be called from destructors
 * @note Resources are nullified after cleanup to prevent double-free
 * @note Logs device removal for debugging and monitoring purposes
 */
void KeyboardDevice::cleanup() noexcept {
    if (evdev_) {
        libevdev_free(evdev_);
        evdev_ = nullptr;
    }
    if (fd_ >= 0) {
        close(fd_);
        fd_ = INVALID_FD;
        log_debug("Removed keyboard: " + path_);
    }
}

/**
 * @brief Static helper to validate if a libevdev device is a keyboard
 * @param dev libevdev device context to check
 * @return true if device supports keyboard input events, false otherwise
 *
 * Performs validation checks to ensure the device:
 * - Has a valid libevdev context
 * - Supports EV_KEY event type (key press/release events)
 * - Has at least one keyboard key (KEY_A used as representative)
 *
 * This filtering helps exclude devices like mice, touchpads, or other
 * input devices that may generate key events but aren't keyboards.
 *
 * @note Uses KEY_A as a representative keyboard key for validation
 * @note Returns false for null device contexts
 */
bool KeyboardDevice::is_keyboard_device(libevdev* dev) noexcept {
    return dev && libevdev_has_event_type(dev, EV_KEY) &&
           libevdev_has_event_code(dev, EV_KEY, KEY_A);
}

//-----------------------------------------------------------------------------
// DeviceMonitor implementation
//-----------------------------------------------------------------------------

/**
 * @brief Initialize udev context and monitor for input device events
 *
 * Sets up the udev monitoring system for detecting keyboard device
 * hot-plug events. The initialization process:
 * 1. Creates udev context for device enumeration and monitoring
 * 2. Sets up monitor for the input subsystem
 * 3. Configures event source filtering
 * 4. Enables monitoring to start receiving events
 *
 * If any step fails, the monitor is marked as invalid but remains safe to use.
 * Error details are logged for debugging purposes.
 *
 * @note Monitor filters events to input subsystem devices only
 * @note Event source is set to "udev" for kernel-generated events
 * @note Failed initialization leaves object in safe, invalid state
 */
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

/**
 * @brief Destructor - automatically clean up udev resources
 *
 * Ensures proper cleanup of udev monitor and context to prevent
 * resource leaks. Safe to call on invalid monitors.
 */
DeviceMonitor::~DeviceMonitor() {
    cleanup();
}

/**
 * @brief Internal cleanup function for safe udev resource deallocation
 *
 * Safely releases udev monitor and context resources. This function is
 * safe to call multiple times and with partially initialized objects.
 *
 * @note Function is noexcept to ensure it can be called from destructors
 * @note Resources are nullified after cleanup to prevent double-free
 */
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

/**
 * @brief Enumerate all existing keyboard devices in the system
 * @return Vector of valid KeyboardDevice objects
 *
 * Scans the system for input devices using udev enumeration and creates
 * KeyboardDevice objects for all detected keyboards. The function:
 * 1. Creates udev enumerate context
 * 2. Filters for input subsystem devices
 * 3. Iterates through all matching devices
 * 4. Validates device nodes for keyboard characteristics
 * 5. Creates KeyboardDevice objects for valid keyboards
 *
 * Invalid devices are automatically filtered out during the process.
 * This is typically called during initialization to discover existing devices.
 *
 * @note Only returns devices that pass keyboard validation checks
 * @note Each returned device is guaranteed to be valid (is_valid() == true)
 * @note Returns empty vector if udev context is invalid
 */
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

/**
 * @brief Process pending hot-plug events from udev monitor
 * @param on_add Callback function called when keyboard is connected
 * @param on_remove Callback function called when keyboard is disconnected
 * @return true if any events were processed, false otherwise
 *
 * Reads events from the udev monitor and calls appropriate callbacks
 * for device addition and removal. The function:
 * 1. Checks for pending udev events on the monitor
 * 2. Processes each event to determine action type
 * 3. Filters events for keyboard-relevant devices
 * 4. Calls appropriate callback for add/remove actions
 * 5. Continues until no more events are pending
 *
 * This should be called when the monitor file descriptor becomes readable.
 *
 * @section CallbackParameters Callback Parameters
 * - on_add: Called with device path string (e.g., "/dev/input/event0")
 * - on_remove: Called with device path string of removed device
 *
 * @note Callbacks may be called multiple times per call to process_events()
 * @note Callbacks should be lightweight to avoid blocking event processing
 * @note Returns false if monitor is invalid
 */
bool DeviceMonitor::process_events(const std::function<void(const std::string&)>& on_add,
                                   const std::function<void(const std::string&)>& on_remove) const {
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

/**
 * @brief Initialize keyboard manager and discover existing devices
 *
 * Creates device monitor and enumerates all existing keyboard devices
 * in the system. The manager will be invalid if monitor initialization fails.
 *
 * During initialization:
 * 1. Creates DeviceMonitor for hot-plug event detection
 * 2. Enumerates all existing keyboard devices in the system
 * 3. Validates and stores successfully initialized devices
 * 4. Logs the number of discovered keyboards for debugging
 *
 * @note The manager will be invalid if monitor initialization fails
 * @note Only valid keyboard devices are stored in the collection
 */
KeyboardManager::KeyboardManager() {
    if (monitor_.is_valid()) {
        keyboards_ = monitor_.enumerate_keyboards();
        log_info("Found " + std::to_string(keyboards_.size()) + " keyboard(s) at startup");
    }
}

/**
 * @brief Process hot-plug events and update device collection
 * @return true if the device list was modified (devices added or removed)
 *
 * Checks for pending hot-plug events and updates the internal device
 * collection accordingly. The function:
 * 1. Validates the monitor is ready for event processing
 * 2. Sets up callbacks for device addition and removal
 * 3. Processes all pending events from the udev monitor
 * 4. Updates the devices_changed flag for each modification
 * 5. Returns whether the device collection was modified
 *
 * Should be called regularly to maintain an accurate device list.
 *
 * @section EventHandling Event Handling
 * - Device addition: Creates new KeyboardDevice and adds to collection
 * - Device removal: Removes corresponding device from collection
 * - Invalid devices: Automatically filtered out during processing
 *
 * @note This function is typically called in the main event loop
 * @note Returns true if polling file descriptor list should be rebuilt
 * @note Returns false if monitor is invalid
 */
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

/**
 * @brief Get file descriptors for poll() operations
 * @return Vector containing all device FDs plus monitor FD
 *
 * Returns a vector of file descriptors suitable for use with poll()
 * or select(). The function:
 * 1. Pre-allocates vector space for efficiency
 * 2. Iterates through all managed keyboard devices
 * 3. Adds file descriptors from valid devices only
 * 4. Appends the monitor file descriptor for hot-plug events
 * 5. Returns the complete list for polling operations
 *
 * The vector includes file descriptors for all valid keyboard devices
 * plus the hot-plug monitor file descriptor.
 *
 * @section PollingUsage Polling Usage
 * @code
 * auto fds = manager.get_poll_fds();
 * std::vector<pollfd> pollfds;
 * for (int fd : fds) {
 *     pollfds.push_back({fd, POLLIN, 0});
 * }
 * int result = poll(pollfds.data(), pollfds.size(), timeout);
 * @endcode
 *
 * @note The last file descriptor in the vector is always the monitor FD
 * @note Only valid devices are included in the returned vector
 * @note Vector should be rebuilt after calling update_devices() if it returns true
 */
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

/**
 * @brief Add new keyboard device to managed collection
 * @param device_path Path to the new input device
 *
 * Creates a KeyboardDevice for the specified path and adds it to
 * the collection if it's valid. The function:
 * 1. Checks if device already exists in the collection
 * 2. Returns early if device is already managed
 * 3. Creates new KeyboardDevice from the path
 * 4. Validates the device initialization succeeded
 * 5. Adds the device to the collection if valid
 *
 * Invalid devices are ignored and not added to the collection.
 * Duplicate devices are detected and not added again.
 *
 * @note Only valid devices are added to the collection
 * @note Duplicate devices are safely ignored
 */
void KeyboardManager::add_device(const std::string& device_path) {
    // Check if device already exists
    auto it = std::find_if(
        keyboards_.begin(), keyboards_.end(),
        [&device_path](const KeyboardDevice& kbd) { return kbd.path() == device_path; });

    if (it != keyboards_.end()) {
        return;  // Already exists
    }

    KeyboardDevice kbd(device_path);
    if (kbd.is_valid()) {
        keyboards_.emplace_back(std::move(kbd));
    }
}

/**
 * @brief Remove keyboard device from managed collection
 * @param device_path Path of the device to remove
 *
 * Finds and removes the device with the specified path from the
 * collection. The function:
 * 1. Searches for device with matching path
 * 2. Uses lambda predicate for path comparison
 * 3. Removes device from collection if found
 * 4. Device destructor handles resource cleanup automatically
 *
 * If the device is not found, no action is taken.
 * The device's destructor will handle cleanup of resources.
 *
 * @note Device cleanup is handled automatically by RAII
 * @note Safe to call with non-existent device paths
 */
void KeyboardManager::remove_device(const std::string& device_path) {
    auto it = std::find_if(
        keyboards_.begin(), keyboards_.end(),
        [&device_path](const KeyboardDevice& kbd) { return kbd.path() == device_path; });

    if (it != keyboards_.end()) {
        keyboards_.erase(it);
    }
}

}  // namespace device
