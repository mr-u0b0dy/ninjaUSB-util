/**
 * @file hid_keycodes.hpp
 * @brief HID keyboard mapping and report generation utilities
 * @author Dharun A P
 * @date 2025
 * @copyright Copyright (c) 2025 Dharun A P
 * @license SPDX-License-Identifier: Apache-2.0
 * 
 * This module provides comprehensive mapping between Linux input event codes
 * and USB HID usage codes, along with utilities for generating standard HID
 * keyboard reports. It supports both standard keyboard input and consumer
 * control functions.
 * 
 * @section HIDStandard HID Standard Compliance
 * This implementation follows the USB HID specification for keyboard devices:
 * - Keyboard/Keypad Usage Page (0x07) for standard keys
 * - Consumer Control Usage Page (0x0C) for media keys
 * - Standard 8-byte keyboard report format
 * - Modifier key handling according to HID specification
 * 
 * @section ReportFormat Keyboard Report Format
 * Standard HID keyboard reports are 8 bytes:
 * - Byte 0: Modifier keys bitmap (Ctrl, Alt, Shift, etc.)
 * - Byte 1: Reserved (always 0)
 * - Bytes 2-7: Up to 6 simultaneous key codes (non-modifier keys)
 * 
 * @section KeyMapping Key Mapping
 * The module provides bidirectional mapping:
 * - Linux KEY_* codes → USB HID usage codes
 * - Support for 104-key US layout
 * - Function keys, arrow keys, and keypad
 * - Modifier key detection and handling
 * - Media and consumer control keys
 * 
 * @section Usage Usage Example
 * @code
 * hid::KeyboardState state;
 * 
 * // Process key press
 * state.press_key(KEY_A);
 * state.press_key(KEY_LEFTCTRL);
 * 
 * // Generate HID report
 * auto report = state.generate_report();
 * 
 * // Process key release
 * state.release_key(KEY_A);
 * @endcode
 */

#pragma once

// ---------------------------------------------------------------------------
//  HID Keycode Maps & Report Generation
// ---------------------------------------------------------------------------
//  * Linux KEY_* → USB HID Usage IDs (Keyboard/Keypad page 0x07)
//  * Linux KEY_* → Consumer‑Control Usage IDs (Consumer page 0x0C)
//  * Lightweight helper utilities to build 8‑byte keyboard reports and
//    2‑byte consumer‑control reports.
//  * Complete modifier key handling and key state management
// ---------------------------------------------------------------------------

#include <linux/input-event-codes.h>  // Linux KEY_* codes
#include <array>
#include <cstdint>
#include <set>
#include <unordered_map>
#include <optional>

/**
 * @namespace hid
 * @brief HID keyboard mapping and report generation functionality
 * 
 * This namespace contains utilities for converting Linux input events
 * to USB HID keyboard reports, including key mapping tables, state
 * management, and report formatting according to HID specifications.
 */
namespace hid {

// ---------------------------------------------------------------------------
//  HID Report Constants and Specifications
// ---------------------------------------------------------------------------

/**
 * @brief Size of standard HID keyboard report in bytes
 * 
 * Standard HID keyboard reports are exactly 8 bytes according to the
 * USB HID specification for boot keyboard devices.
 */
constexpr std::size_t KEYBOARD_REPORT_SIZE = 8;

/**
 * @brief Size of HID consumer control report in bytes  
 * 
 * Consumer control reports for media keys are 2 bytes.
 */
constexpr std::size_t CONSUMER_REPORT_SIZE = 2;

/**
 * @brief Maximum number of simultaneous non-modifier keys
 * 
 * HID keyboard reports can contain up to 6 simultaneous key presses
 * (bytes 2-7 of the 8-byte report), excluding modifier keys.
 */
constexpr std::size_t MAX_SIMULTANEOUS_KEYS = 6;

/**
 * @brief Base HID usage code for modifier keys
 * 
 * Modifier keys (Ctrl, Alt, Shift, etc.) start at usage code 0xE0
 * and extend to 0xE7, covering 8 possible modifier keys.
 */
constexpr std::uint8_t MODIFIER_BASE = 0xE0;

/**
 * @brief Maximum HID usage code for modifier keys
 * 
 * The highest modifier key usage code is 0xE7 (Right GUI/Windows key).
 */
constexpr std::uint8_t MODIFIER_MAX = 0xE7;

// ---------------------------------------------------------------------------
//  Key Mapping Tables
// ---------------------------------------------------------------------------

/**
 * @brief Mapping from Linux KEY_* codes to USB HID keyboard usage codes
 * 
 * This table provides conversion from Linux input event key codes to
 * USB HID usage codes for the Keyboard/Keypad usage page (0x07).
 * 
 * @section Coverage Key Coverage
 * - Complete US 104-key layout
 * - Alphabetic keys (A-Z)
 * - Numeric keys (0-9) and symbols
 * - Function keys (F1-F12)
 * - Arrow keys and navigation
 * - Keypad numbers and operators
 * - Modifier keys (Ctrl, Alt, Shift, etc.)
 * - Special keys (Space, Tab, Enter, etc.)
 * 
 * @section Standard HID Standard Reference
 * Usage codes follow the USB HID Usage Tables specification,
 * Keyboard/Keypad Page (0x07). See:
 * https://www.usb.org/sites/default/files/documents/hut1_12v2.pdf
 * 
 * @note Only includes keys that have direct HID equivalents
 * @note Modifier keys are handled separately in the modifier bitmap
 */
inline const std::unordered_map<int, std::uint8_t> kKeyboardUsage = {
    /* Alphabet */
    {KEY_A, 0x04}, {KEY_B, 0x05}, {KEY_C, 0x06}, {KEY_D, 0x07},
    {KEY_E, 0x08}, {KEY_F, 0x09}, {KEY_G, 0x0A}, {KEY_H, 0x0B},
    {KEY_I, 0x0C}, {KEY_J, 0x0D}, {KEY_K, 0x0E}, {KEY_L, 0x0F},
    {KEY_M, 0x10}, {KEY_N, 0x11}, {KEY_O, 0x12}, {KEY_P, 0x13},
    {KEY_Q, 0x14}, {KEY_R, 0x15}, {KEY_S, 0x16}, {KEY_T, 0x17},
    {KEY_U, 0x18}, {KEY_V, 0x19}, {KEY_W, 0x1A}, {KEY_X, 0x1B},
    {KEY_Y, 0x1C}, {KEY_Z, 0x1D},

    /* Number row */
    {KEY_1, 0x1E}, {KEY_2, 0x1F}, {KEY_3, 0x20}, {KEY_4, 0x21},
    {KEY_5, 0x22}, {KEY_6, 0x23}, {KEY_7, 0x24}, {KEY_8, 0x25},
    {KEY_9, 0x26}, {KEY_0, 0x27},

    /* Punctuation / symbols */
    {KEY_ENTER, 0x28},  {KEY_ESC, 0x29},        {KEY_BACKSPACE, 0x2A},
    {KEY_TAB, 0x2B},    {KEY_SPACE, 0x2C},      {KEY_MINUS, 0x2D},
    {KEY_EQUAL, 0x2E},  {KEY_LEFTBRACE, 0x2F},  {KEY_RIGHTBRACE, 0x30},
    {KEY_BACKSLASH, 0x31}, {KEY_SEMICOLON, 0x33}, {KEY_APOSTROPHE, 0x34},
    {KEY_GRAVE, 0x35},  {KEY_COMMA, 0x36},      {KEY_DOT, 0x37},
    {KEY_SLASH, 0x38},  {KEY_CAPSLOCK, 0x39},

    /* Function row */
    {KEY_F1, 0x3A},  {KEY_F2, 0x3B},  {KEY_F3, 0x3C},  {KEY_F4, 0x3D},
    {KEY_F5, 0x3E},  {KEY_F6, 0x3F},  {KEY_F7, 0x40},  {KEY_F8, 0x41},
    {KEY_F9, 0x42},  {KEY_F10, 0x43}, {KEY_F11, 0x44}, {KEY_F12, 0x45},

    /* Print/Scroll/Pause */
    {KEY_SYSRQ, 0x46},      {KEY_SCROLLLOCK, 0x47}, {KEY_PAUSE, 0x48},

    /* Insert/Delete/Home/End/PgUp/PgDn */
    {KEY_INSERT, 0x49}, {KEY_HOME, 0x4A}, {KEY_PAGEUP, 0x4B},
    {KEY_DELETE, 0x4C}, {KEY_END, 0x4D},  {KEY_PAGEDOWN, 0x4E},

    /* Arrow keys */
    {KEY_RIGHT, 0x4F}, {KEY_LEFT, 0x50}, {KEY_DOWN, 0x51}, {KEY_UP, 0x52},

    /* Keypad */
    {KEY_NUMLOCK, 0x53}, {KEY_KPSLASH, 0x54},   {KEY_KPASTERISK, 0x55},
    {KEY_KPMINUS, 0x56}, {KEY_KPPLUS, 0x57},    {KEY_KPENTER, 0x58},
    {KEY_KP1, 0x59}, {KEY_KP2, 0x5A}, {KEY_KP3, 0x5B},
    {KEY_KP4, 0x5C}, {KEY_KP5, 0x5D}, {KEY_KP6, 0x5E},
    {KEY_KP7, 0x5F}, {KEY_KP8, 0x60}, {KEY_KP9, 0x61},
    {KEY_KP0, 0x62}, {KEY_KPDOT, 0x63}, {KEY_KPEQUAL, 0x67},

    /* ISO / Intl */
    {KEY_102ND, 0x64}, // Additional key for non-US keyboards

    /* Modifiers */
    {KEY_LEFTCTRL, 0xE0},  {KEY_LEFTSHIFT, 0xE1}, {KEY_LEFTALT, 0xE2},
    {KEY_LEFTMETA, 0xE3},  {KEY_RIGHTCTRL, 0xE4}, {KEY_RIGHTSHIFT, 0xE5},
    {KEY_RIGHTALT, 0xE6},  {KEY_RIGHTMETA, 0xE7},

    /* Misc (application/system) */
    {KEY_MENU, 0x65}, {KEY_POWER, 0x66}, {KEY_SLEEP, 0x68}, {KEY_WAKEUP, 0x69},
};

inline const std::unordered_map<int, std::uint16_t> kConsumerUsage = {
    {KEY_VOLUMEUP, 0x00E9},   {KEY_VOLUMEDOWN, 0x00EA}, {KEY_MUTE, 0x00E2},
    {KEY_PLAYPAUSE, 0x00CD},  {KEY_NEXTSONG, 0x00B5},   {KEY_PREVIOUSSONG, 0x00B6},
    {KEY_STOPCD, 0x00B7},     {KEY_EJECTCD, 0x00B8},
    {KEY_BRIGHTNESSUP, 0x006F}, {KEY_BRIGHTNESSDOWN, 0x0070},
    {KEY_HOMEPAGE, 0x0223}, {KEY_SEARCH, 0x0221}, {KEY_BACK, 0x0224},
    {KEY_FORWARD, 0x0225}, {KEY_REFRESH, 0x0227}, {KEY_BOOKMARKS, 0x022A},
};

// ---------------------------------------------------------------------------
//  Helper utilities
// ---------------------------------------------------------------------------
[[nodiscard]] constexpr bool is_modifier(std::uint8_t hid_code) noexcept {
    return hid_code >= MODIFIER_BASE && hid_code <= MODIFIER_MAX;
}

[[nodiscard]] constexpr std::uint8_t modifier_bit(std::uint8_t hid_code) noexcept {
    return static_cast<std::uint8_t>(1u << (hid_code - MODIFIER_BASE));
}

/**
 * @brief Look up HID usage code for a Linux key code
 * @param linux_code Linux input event code
 * @return HID usage code if found, nullopt otherwise
 */
[[nodiscard]] inline std::optional<std::uint8_t> get_keyboard_usage(int linux_code) noexcept {
    if (const auto it = kKeyboardUsage.find(linux_code); it != kKeyboardUsage.end()) {
        return it->second;
    }
    return std::nullopt;
}

/**
 * @brief Look up consumer control usage code for a Linux key code
 * @param linux_code Linux input event code
 * @return Consumer usage code if found, nullopt otherwise
 */
[[nodiscard]] inline std::optional<std::uint16_t> get_consumer_usage(int linux_code) noexcept {
    if (const auto it = kConsumerUsage.find(linux_code); it != kConsumerUsage.end()) {
        return it->second;
    }
    return std::nullopt;
}

/**
 * @brief Represents the state of a HID keyboard
 */
class KeyboardState {
private:
    std::uint8_t modifiers_{0};
    std::set<std::uint8_t> pressed_keys_;   // Non‑modifier HID codes
    mutable std::array<std::uint8_t, KEYBOARD_REPORT_SIZE> report_{};
    mutable bool report_dirty_{true};

    void update_report() const noexcept {
        if (!report_dirty_) return;
        
        report_.fill(0);
        report_[0] = modifiers_;
        
        std::size_t idx = 2;
        for (const auto key : pressed_keys_) {
            if (idx < report_.size()) {
                report_[idx++] = key;
            } else {
                // Handle key rollover - could implement NKRO or 6KRO behavior
                break;
            }
        }
        report_dirty_ = false;
    }

public:
    /**
     * @brief Get the current 8-byte HID keyboard report
     * @return Reference to the report array
     */
    [[nodiscard]] const std::array<std::uint8_t, KEYBOARD_REPORT_SIZE>& get_report() const noexcept {
        update_report();
        return report_;
    }

    /**
     * @brief Get current modifier state
     * @return Modifier byte
     */
    [[nodiscard]] std::uint8_t get_modifiers() const noexcept {
        return modifiers_;
    }

    /**
     * @brief Get number of currently pressed non-modifier keys
     * @return Number of pressed keys
     */
    [[nodiscard]] std::size_t get_pressed_key_count() const noexcept {
        return pressed_keys_.size();
    }

    /**
     * @brief Check if keyboard state has changed since last report
     * @return True if report needs to be sent
     */
    [[nodiscard]] bool is_dirty() const noexcept {
        return report_dirty_;
    }

    /**
     * @brief Apply a key press/release event
     * @param hid_code HID usage code
     * @param pressed True for press, false for release
     */
    void set_key_state(std::uint8_t hid_code, bool pressed) noexcept {
        if (is_modifier(hid_code)) {
            const auto bit = modifier_bit(hid_code);
            if (pressed) {
                modifiers_ |= bit;
            } else {
                modifiers_ &= ~bit;
            }
        } else {
            if (pressed) {
                pressed_keys_.insert(hid_code);
            } else {
                pressed_keys_.erase(hid_code);
            }
        }
        report_dirty_ = true;
    }

    /**
     * @brief Clear all pressed keys and modifiers
     */
    void clear() noexcept {
        modifiers_ = 0;
        pressed_keys_.clear();
        report_dirty_ = true;
    }
};

/**
 * @brief Apply a Linux EV_KEY event to the keyboard state
 * @param state Keyboard state to modify
 * @param linux_code Linux input event code
 * @param value Event value (0=release, 1=press, 2=repeat)
 * @return true if handled as keyboard key, false if not a keyboard key
 */
[[nodiscard]] inline bool apply_key_event(KeyboardState& state, int linux_code, int value) noexcept {
    const auto hid_code = get_keyboard_usage(linux_code);
    if (!hid_code) {
        return false;  // Not a keyboard usage
    }

    switch (value) {
        case 0:  // Key release
            state.set_key_state(*hid_code, false);
            break;
        case 1:  // Key press
        case 2:  // Key repeat
            state.set_key_state(*hid_code, true);
            break;
        default:
            return false;  // Unknown value
    }

    return true;
}

/**
 * @brief Build a 2‑byte consumer‑control report (little‑endian)
 * @param linux_code Linux input event code
 * @param value Event value (0=release, 1=press)
 * @return 2-byte consumer report
 */
[[nodiscard]] inline std::array<std::uint8_t, CONSUMER_REPORT_SIZE> 
make_consumer_report(int linux_code, int value) noexcept {
    std::uint16_t usage = 0;  // Default: empty / key‑release
    
    if (value == 1) {
        if (const auto consumer_usage = get_consumer_usage(linux_code)) {
            usage = *consumer_usage;
        }
    }
    
    return {
        static_cast<std::uint8_t>(usage & 0xFF), 
        static_cast<std::uint8_t>((usage >> 8) & 0xFF)
    };
}

} // namespace hid
