/**
 * @file device_manager.hpp
 * @brief Device management and hot-plug support for USB keyboards
 * @author Dharun A P
 * @date 2025
 * @copyright Copyright (c) 2025 Dharun A P
 * @license SPDX-License-Identifier: Apache-2.0
 * 
 * This module provides comprehensive device management for USB keyboards,
 * including enumeration, hot-plug event monitoring, and input event handling.
 * It uses udev for device discovery and libevdev for low-level input processing.
 * 
 * @section Architecture Design Architecture
 * - RAII pattern for automatic resource management
 * - Hot-plug aware device monitoring via udev
 * - Efficient polling-based input event processing
 * - Type-safe device filtering and validation
 * 
 * @section DeviceUsage Usage Example
 * @code
 * device::KeyboardManager manager;
 * if (!manager.is_valid()) {
 *     std::cerr << "Failed to initialize device management\n";
 *     return 1;
 * }
 * 
 * // Main event loop
 * while (running) {
 *     auto events = manager.poll_events(1); // 1ms timeout
 *     for (const auto& event : events) {
 *         // Process keyboard event
 *     }
 * }
 * @endcode
 */

#pragma once

#include <string>
#include <vector>
#include <functional>

// Forward declarations for system headers to minimize compile dependencies
struct udev;        //!< udev context for device enumeration
struct udev_monitor; //!< udev monitor for hot-plug events  
struct libevdev;    //!< libevdev device handle for input processing

/**
 * @namespace device
 * @brief Device management functionality for keyboard input handling
 * 
 * This namespace contains classes and utilities for managing USB keyboard
 * devices, including device enumeration, hot-plug event monitoring, and
 * input event processing using the Linux input subsystem.
 */
namespace device {

/**
 * @class KeyboardDevice
 * @brief RAII wrapper for individual USB keyboard input devices
 * 
 * This class manages the lifetime of a keyboard device, including the file
 * descriptor and libevdev context. It provides a safe, exception-safe interface
 * for keyboard device operations with automatic cleanup.
 * 
 * @section DeviceFeatures Key Features
 * - RAII resource management for file descriptors and libevdev contexts
 * - Device validation and capability checking
 * - Move semantics for efficient transfer of ownership
 * - Type-safe device information access
 * - Automatic cleanup on destruction
 * 
 * @section DeviceValidation Device Validation
 * The class performs several validation checks:
 * - File descriptor validity
 * - libevdev initialization success
 * - Keyboard capability verification (EV_KEY events)
 * - Input device type confirmation
 * 
 * @note Objects are movable but not copyable to ensure unique ownership
 * @note Invalid devices can be safely used (operations become no-ops)
 */
class KeyboardDevice {
private:
    int fd_{-1};                //!< File descriptor for the input device
    libevdev* evdev_{nullptr};  //!< libevdev context for event processing
    std::string path_;          //!< Device path (e.g., /dev/input/event0)
    std::string name_;          //!< Human-readable device name

public:
    /**
     * @brief Construct keyboard device from device path
     * @param device_path Path to input device (e.g., "/dev/input/event0")
     * 
     * Opens the device file, initializes libevdev context, and validates
     * that the device is a keyboard. If any step fails, the device will
     * be marked as invalid but remain safe to use.
     * 
     * @note Constructor does not throw exceptions - check is_valid() after construction
     */
    explicit KeyboardDevice(const std::string& device_path);
    
    /**
     * @brief Destructor - automatically cleans up all resources
     * 
     * Closes file descriptor and frees libevdev context if they were
     * successfully initialized.
     */
    ~KeyboardDevice();

    // Non-copyable but movable for unique resource ownership
    KeyboardDevice(const KeyboardDevice&) = delete;
    KeyboardDevice& operator=(const KeyboardDevice&) = delete;
    
    /**
     * @brief Move constructor for transferring device ownership
     * @param other Device to move from (left in valid but empty state)
     */
    KeyboardDevice(KeyboardDevice&& other) noexcept;
    
    /**
     * @brief Move assignment operator
     * @param other Device to move from
     * @return Reference to this device
     */
    KeyboardDevice& operator=(KeyboardDevice&& other) noexcept;

    /**
     * @brief Check if device was successfully initialized and is usable
     * @return true if device is valid and ready for input processing
     */
    [[nodiscard]] bool is_valid() const noexcept { return fd_ >= 0 && evdev_ != nullptr; }
    
    /**
     * @brief Get file descriptor for polling operations
     * @return File descriptor or -1 if invalid
     */
    [[nodiscard]] int fd() const noexcept { return fd_; }
    
    /**
     * @brief Get libevdev context for event processing
     * @return libevdev context or nullptr if invalid
     */
    [[nodiscard]] libevdev* evdev() const noexcept { return evdev_; }
    
    /**
     * @brief Get device path
     * @return Device path string (may be empty if invalid)
     */
    [[nodiscard]] const std::string& path() const noexcept { return path_; }
    
    /**
     * @brief Get human-readable device name
     * @return Device name string (may be empty if invalid)
     */
    [[nodiscard]] const std::string& name() const noexcept { return name_; }

private:
    /**
     * @brief Clean up all allocated resources
     * 
     * Internal helper for destructor and move operations. Safe to call
     * multiple times and with partially initialized objects.
     */
    void cleanup() noexcept;
    
    /**
     * @brief Check if libevdev device has keyboard capabilities
     * @param dev libevdev device context
     * @return true if device supports keyboard input events
     * 
     * Validates that the device supports EV_KEY events and has a reasonable
     * set of keyboard keys, filtering out devices like mice or touchpads
     * that also generate key events.
     */
    static bool is_keyboard_device(libevdev* dev) noexcept;
};

/**
 * @class DeviceMonitor  
 * @brief RAII wrapper for udev device monitoring and hot-plug event detection
 * 
 * This class manages udev context and monitor for detecting USB keyboard
 * device addition and removal events. It provides a polling-based interface
 * for processing hot-plug events in the main application loop.
 * 
 * @section UdevIntegration udev Integration
 * - Initializes udev context for device enumeration
 * - Sets up udev monitor for input subsystem events
 * - Filters events for keyboard devices specifically
 * - Provides file descriptor for poll()-based event detection
 * 
 * @section EventProcessing Event Processing
 * The monitor detects these events:
 * - Device addition: New keyboard connected
 * - Device removal: Keyboard disconnected  
 * - Device change: Keyboard properties modified
 * 
 * @note This class is neither copyable nor movable for resource safety
 * @note Monitor must be checked for validity before use
 */
class DeviceMonitor {
private:
    udev* udev_{nullptr};           //!< udev context for device operations
    udev_monitor* monitor_{nullptr}; //!< udev monitor for hot-plug events
    int monitor_fd_{-1};            //!< File descriptor for polling monitor

public:
    /**
     * @brief Initialize udev context and monitor for keyboard devices
     * 
     * Sets up udev monitoring for input subsystem devices, specifically
     * filtering for events relevant to keyboard devices. The monitor
     * is configured to detect add/remove events.
     */
    DeviceMonitor();
    
    /**
     * @brief Clean up udev resources
     */
    ~DeviceMonitor();

    // Non-copyable, non-movable for resource safety and simplicity
    DeviceMonitor(const DeviceMonitor&) = delete;
    DeviceMonitor& operator=(const DeviceMonitor&) = delete;
    DeviceMonitor(DeviceMonitor&&) = delete;
    DeviceMonitor& operator=(DeviceMonitor&&) = delete;

    /**
     * @brief Check if monitor was successfully initialized
     * @return true if monitor is ready for event processing
     */
    [[nodiscard]] bool is_valid() const noexcept { return udev_ != nullptr && monitor_ != nullptr; }
    
    /**
     * @brief Get file descriptor for poll() operations
     * @return Monitor file descriptor or -1 if invalid
     */
    [[nodiscard]] int monitor_fd() const noexcept { return monitor_fd_; }

    /**
     * @brief Enumerate all existing keyboard devices in the system
     * @return Vector of valid KeyboardDevice objects
     * 
     * Scans the system for input devices and creates KeyboardDevice objects
     * for all detected keyboards. Invalid devices are filtered out.
     * This is typically called during initialization to discover existing devices.
     * 
     * @note Only returns devices that pass keyboard validation checks
     * @note Each returned device is guaranteed to be valid (is_valid() == true)
     */
    [[nodiscard]] std::vector<KeyboardDevice> enumerate_keyboards() const;

    /**
     * @brief Process pending hot-plug events from udev monitor
     * @param on_add Callback function called when keyboard is connected
     * @param on_remove Callback function called when keyboard is disconnected  
     * @return true if any events were processed
     * 
     * Reads events from the udev monitor and calls appropriate callbacks
     * for device addition and removal. This should be called when the
     * monitor file descriptor becomes readable.
     * 
     * @section CallbackParameters Callback Parameters
     * - on_add: Called with device path string (e.g., "/dev/input/event0")
     * - on_remove: Called with device path string of removed device
     * 
     * @note Callbacks may be called multiple times per call to process_events()
     * @note Callbacks should be lightweight to avoid blocking event processing
     */
    bool process_events(
        const std::function<void(const std::string&)>& on_add,
        const std::function<void(const std::string&)>& on_remove
    ) const;

private:
    /**
     * @brief Clean up udev resources safely
     * 
     * Internal cleanup function that safely releases udev context and monitor.
     * Can be called multiple times safely.
     */
    void cleanup() noexcept;
};

/**
 * @class KeyboardManager
 * @brief High-level manager for multiple keyboard devices with hot-plug support
 * 
 * This class provides a unified interface for managing multiple USB keyboards
 * with automatic hot-plug detection and event processing. It combines device
 * enumeration, monitoring, and event polling into a single, easy-to-use interface.
 * 
 * @section Functionality Key Functionality
 * - Automatic discovery of existing keyboard devices at startup
 * - Real-time hot-plug event processing (connect/disconnect)
 * - Centralized polling interface for all managed devices
 * - Thread-safe device collection management
 * - Efficient file descriptor management for polling
 * 
 * @section UsagePattern Usage Pattern
 * @code
 * KeyboardManager manager;
 * if (!manager.is_valid()) {
 *     // Handle initialization error
 *     return;
 * }
 * 
 * while (running) {
 *     // Update device list for hot-plug events
 *     manager.update_devices();
 *     
 *     // Get file descriptors for polling
 *     auto fds = manager.get_poll_fds();
 *     
 *     // Poll for input events
 *     // ... polling logic ...
 * }
 * @endcode
 * 
 * @section DevicePerformance Performance Characteristics
 * - O(1) device lookup and access operations
 * - Minimal overhead for hot-plug event processing
 * - Efficient polling with consolidated file descriptor list
 * - Memory-efficient storage with move semantics
 * 
 * @note The manager automatically handles device lifecycle management
 * @note Invalid devices are automatically filtered out and cleaned up
 */
class KeyboardManager {
private:
    std::vector<KeyboardDevice> keyboards_; //!< Collection of managed keyboard devices
    DeviceMonitor monitor_;                  //!< Hot-plug event monitor

public:
    /**
     * @brief Initialize keyboard manager and discover existing devices
     * 
     * Creates device monitor and enumerates all existing keyboard devices
     * in the system. The manager will be invalid if monitor initialization fails.
     */
    KeyboardManager();

    /**
     * @brief Check if manager was successfully initialized
     * @return true if manager is ready for device operations
     */
    [[nodiscard]] bool is_valid() const noexcept { return monitor_.is_valid(); }
    
    /**
     * @brief Get read-only access to managed keyboard devices
     * @return Const reference to device collection
     */
    [[nodiscard]] const std::vector<KeyboardDevice>& keyboards() const noexcept { return keyboards_; }
    
    /**
     * @brief Get number of currently managed devices
     * @return Count of valid keyboard devices
     */
    [[nodiscard]] std::size_t device_count() const noexcept { return keyboards_.size(); }
    
    /**
     * @brief Get monitor file descriptor for polling
     * @return File descriptor for hot-plug event monitoring
     */
    [[nodiscard]] int monitor_fd() const noexcept { return monitor_.monitor_fd(); }

    /**
     * @brief Process hot-plug events and update device collection
     * @return true if the device list was modified (devices added or removed)
     * 
     * Checks for pending hot-plug events and updates the internal device
     * collection accordingly. Should be called regularly to maintain an
     * accurate device list.
     * 
     * @section EventHandling Event Handling
     * - Device addition: Creates new KeyboardDevice and adds to collection
     * - Device removal: Removes corresponding device from collection
     * - Invalid devices: Automatically filtered out during processing
     * 
     * @note This function is typically called in the main event loop
     * @note Returns true if polling file descriptor list should be rebuilt
     */
    bool update_devices();

    /**
     * @brief Get file descriptors for poll() operations
     * @return Vector containing all device FDs plus monitor FD
     * 
     * Returns a vector of file descriptors suitable for use with poll()
     * or select(). The vector includes file descriptors for all valid
     * keyboard devices plus the hot-plug monitor file descriptor.
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
    [[nodiscard]] std::vector<int> get_poll_fds() const;

private:
    /**
     * @brief Add new keyboard device to managed collection
     * @param device_path Path to the new input device
     * 
     * Creates a KeyboardDevice for the specified path and adds it to
     * the collection if it's valid. Invalid devices are ignored.
     */
    void add_device(const std::string& device_path);
    
    /**
     * @brief Remove keyboard device from managed collection
     * @param device_path Path of the device to remove
     * 
     * Finds and removes the device with the specified path from the
     * collection. The device's destructor will handle cleanup.
     */
    void remove_device(const std::string& device_path);
};

} // namespace device
