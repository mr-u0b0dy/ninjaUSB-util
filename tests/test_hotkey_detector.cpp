/**
 * @file test_hotkey_detector.cpp
 * @brief Unit tests for exit hotkey detection functionality
 * @author Dharun A P
 * @license SPDX-License-Identifier: Apache-2.0
 * @copyright SPDX-FileCopyrightText: 2025 Dharun A P
 */

#include <cassert>

#include "exit_hotkey_detector.hpp"
#include "test_framework.hpp"

namespace {

void test_individual_keys() {
    ExitHotkeyDetector detector;

    // Test Ctrl key
    bool exit_triggered = detector.process_key_event(KEY_LEFTCTRL, 1);  // Press
    assert(!exit_triggered);
    assert(detector.is_ctrl_pressed());

    exit_triggered = detector.process_key_event(KEY_LEFTCTRL, 0);  // Release
    assert(!exit_triggered);
    assert(!detector.is_ctrl_pressed());

    // Test Alt key
    exit_triggered = detector.process_key_event(KEY_LEFTALT, 1);  // Press
    assert(!exit_triggered);
    assert(detector.is_alt_pressed());

    exit_triggered = detector.process_key_event(KEY_LEFTALT, 0);  // Release
    assert(!exit_triggered);
    assert(!detector.is_alt_pressed());

    // Test H key
    exit_triggered = detector.process_key_event(KEY_H, 1);  // Press
    assert(!exit_triggered);
    assert(detector.is_h_pressed());

    exit_triggered = detector.process_key_event(KEY_H, 0);  // Release
    assert(!exit_triggered);
    assert(!detector.is_h_pressed());
}

void test_partial_combinations() {
    ExitHotkeyDetector detector;

    // Test Ctrl+Alt (without H)
    detector.process_key_event(KEY_LEFTCTRL, 1);
    detector.process_key_event(KEY_LEFTALT, 1);
    bool exit_triggered = detector.process_key_event(KEY_A, 1);  // Press 'A' instead of 'H'
    assert(!exit_triggered);

    // Test Ctrl+H (without Alt)
    ExitHotkeyDetector detector2;
    detector2.process_key_event(KEY_LEFTCTRL, 1);
    exit_triggered = detector2.process_key_event(KEY_H, 1);
    assert(!exit_triggered);

    // Test Alt+H (without Ctrl)
    ExitHotkeyDetector detector3;
    detector3.process_key_event(KEY_LEFTALT, 1);
    exit_triggered = detector3.process_key_event(KEY_H, 1);
    assert(!exit_triggered);

    std::cout << "PASSED\n";
}

void test_full_combination() {
    ExitHotkeyDetector detector;

    // Press Ctrl first
    bool exit_triggered = detector.process_key_event(KEY_LEFTCTRL, 1);
    assert(!exit_triggered);

    // Press Alt second
    exit_triggered = detector.process_key_event(KEY_LEFTALT, 1);
    assert(!exit_triggered);

    // Press H - should trigger exit
    exit_triggered = detector.process_key_event(KEY_H, 1);
    assert(exit_triggered);

    std::cout << "PASSED\n";
}

void test_different_key_orders() {
    // Order: Alt, Ctrl, H
    ExitHotkeyDetector detector1;
    detector1.process_key_event(KEY_LEFTALT, 1);
    detector1.process_key_event(KEY_LEFTCTRL, 1);
    bool exit_triggered = detector1.process_key_event(KEY_H, 1);
    assert(exit_triggered);

    // Order: H, Ctrl, Alt (H pressed first, but no trigger until all are pressed)
    ExitHotkeyDetector detector2;
    detector2.process_key_event(KEY_H, 1);
    assert(!detector2.process_key_event(KEY_LEFTCTRL, 1));
    // At this point, H is already pressed, so when Alt is pressed,
    // we need to simulate the H press again to trigger
    detector2.process_key_event(KEY_LEFTALT, 1);
    // The combination should not trigger because H was pressed before all modifiers
    exit_triggered = detector2.process_key_event(KEY_H, 0);  // Release H
    assert(!exit_triggered);
    exit_triggered = detector2.process_key_event(KEY_H, 1);  // Press H again
    assert(exit_triggered);

    std::cout << "PASSED\n";
}

void test_right_side_modifiers() {
    ExitHotkeyDetector detector;

    // Test with right Ctrl and right Alt
    detector.process_key_event(KEY_RIGHTCTRL, 1);
    detector.process_key_event(KEY_RIGHTALT, 1);
    bool exit_triggered = detector.process_key_event(KEY_H, 1);
    assert(exit_triggered);

    std::cout << "PASSED\n";
}

void test_mixed_modifiers() {
    ExitHotkeyDetector detector;

    // Test with left Ctrl and right Alt
    detector.process_key_event(KEY_LEFTCTRL, 1);
    detector.process_key_event(KEY_RIGHTALT, 1);
    bool exit_triggered = detector.process_key_event(KEY_H, 1);
    assert(exit_triggered);

    std::cout << "PASSED\n";
}

void test_key_release_behavior() {
    ExitHotkeyDetector detector;

    // Set up the combination
    detector.process_key_event(KEY_LEFTCTRL, 1);
    detector.process_key_event(KEY_LEFTALT, 1);
    detector.process_key_event(KEY_H, 1);

    // Release one key
    detector.process_key_event(KEY_LEFTCTRL, 0);

    // Pressing H again should not trigger (Ctrl is released)
    bool exit_triggered = detector.process_key_event(KEY_H, 0);  // Release
    exit_triggered = detector.process_key_event(KEY_H, 1);       // Press again
    assert(!exit_triggered);

    std::cout << "PASSED\n";
}

void test_state_description() {
    ExitHotkeyDetector detector;

    std::string state = detector.get_state_description();
    assert(state.find("Ctrl: OFF") != std::string::npos);
    assert(state.find("Alt: OFF") != std::string::npos);
    assert(state.find("H: OFF") != std::string::npos);

    detector.process_key_event(KEY_LEFTCTRL, 1);
    state = detector.get_state_description();
    assert(state.find("Ctrl: ON") != std::string::npos);
    assert(state.find("Alt: OFF") != std::string::npos);

    std::cout << "PASSED\n";
}

void test_key_repeat() {
    ExitHotkeyDetector detector;

    // Set up the combination
    detector.process_key_event(KEY_LEFTCTRL, 1);
    detector.process_key_event(KEY_LEFTALT, 1);
    bool exit_triggered = detector.process_key_event(KEY_H, 1);
    assert(exit_triggered);

    // Test auto-repeat (value 2) - should not trigger again
    exit_triggered = detector.process_key_event(KEY_H, 2);
    assert(!exit_triggered);

    std::cout << "PASSED\n";
}

}  // namespace

int main() {
    return test_framework::run_test_suite("Exit Hotkey Detector Unit Tests",
                                          {{"individual key tracking", test_individual_keys},
                                           {"partial combinations", test_partial_combinations},
                                           {"full combination", test_full_combination},
                                           {"different key orders", test_different_key_orders},
                                           {"right side modifiers", test_right_side_modifiers},
                                           {"mixed modifiers", test_mixed_modifiers},
                                           {"key release behavior", test_key_release_behavior},
                                           {"state description", test_state_description},
                                           {"key repeat", test_key_repeat}});
}
