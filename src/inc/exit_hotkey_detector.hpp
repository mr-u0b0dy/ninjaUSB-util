/**
 * @file exit_hotkey_detector.hpp
 * @brief Exit hotkey combination detector for program termination
 * @author Dharun A P
 * @license SPDX-License-Identifier: Apache-2.0
 * @copyright SPDX-FileCopyrightText: 2025 Dharun A P
 */

#pragma once

#include <string>

#include <linux/input-event-codes.h>

/**
 * @brief Hotkey combination detector for program exit
 *
 * Detects the Alt+Ctrl+H key combination to provide a safe way to exit
 * the program while capturing keystrokes. This replaces Ctrl+C functionality
 * which is disabled to prevent accidental program termination.
 */
class ExitHotkeyDetector {
  private:
    bool ctrl_pressed_ = false;
    bool alt_pressed_ = false;
    bool h_pressed_ = false;
    bool enable_logging_ = false;

  public:
    /**
     * @brief Constructor with optional logging control
     * @param enable_logging Whether to enable logging when hotkey is detected
     */
    explicit ExitHotkeyDetector(bool enable_logging = false) : enable_logging_(enable_logging) {}
    /**
     * @brief Process a key event and check for exit hotkey combination
     * @param linux_code Linux input event code
     * @param value Event value (0=release, 1=press, 2=repeat)
     * @return true if Alt+Ctrl+H combination is detected
     */
    bool process_key_event(int linux_code, int value) {
        bool is_press = (value == 1);
        bool is_release = (value == 0);

        // Track modifier key states
        switch (linux_code) {
            case KEY_LEFTCTRL:
            case KEY_RIGHTCTRL:
                if (is_press)
                    ctrl_pressed_ = true;
                else if (is_release)
                    ctrl_pressed_ = false;
                break;

            case KEY_LEFTALT:
            case KEY_RIGHTALT:
                if (is_press)
                    alt_pressed_ = true;
                else if (is_release)
                    alt_pressed_ = false;
                break;

            case KEY_H:
                if (is_press) {
                    h_pressed_ = true;
                    // Check if all required keys are pressed
                    if (ctrl_pressed_ && alt_pressed_ && h_pressed_) {
                        return true;
                    }
                } else if (is_release) {
                    h_pressed_ = false;
                }
                break;

            default:
                // Ignore other keys
                break;
        }

        return false;
    }

    /**
     * @brief Get current state description for debugging
     * @return String describing current modifier states
     */
    std::string get_state_description() const {
        return "Ctrl: " + std::string(ctrl_pressed_ ? "ON" : "OFF") +
               ", Alt: " + std::string(alt_pressed_ ? "ON" : "OFF") +
               ", H: " + std::string(h_pressed_ ? "ON" : "OFF");
    }

    // Test helper methods for unit testing
    bool is_ctrl_pressed() const { return ctrl_pressed_; }
    bool is_alt_pressed() const { return alt_pressed_; }
    bool is_h_pressed() const { return h_pressed_; }
};
