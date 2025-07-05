/**
 * @file test_hid_keycodes.cpp
 * @brief Unit tests for HID keyboard mapping and state management
 * @author Dharun A P
 * @license SPDX-License-Identifier: Apache-2.0
 * @copyright SPDX-FileCopyrightText: 2025 Dharun A P
 */

#include <array>
#include <cassert>
#include <iostream>
#include <string>

#include "hid_keycodes.hpp"

namespace {

void test_keyboard_state_basic() {
    std::cout << "Testing KeyboardState basic functionality... ";

    hid::KeyboardState state;

    // Initial state should be empty
    assert(state.get_modifiers() == 0);
    assert(state.get_pressed_key_count() == 0);

    auto report = state.get_report();
    for (int i = 0; i < 8; ++i) {
        assert(report[i] == 0);
    }

    std::cout << "PASSED\n";
}

void test_modifier_keys() {
    std::cout << "Testing modifier key handling... ";

    hid::KeyboardState state;

    // Test Left Ctrl
    state.set_key_state(0xE0, true);  // Left Ctrl
    assert(state.get_modifiers() == 0x01);
    auto report = state.get_report();
    assert(report[0] == 0x01);

    // Test Left Shift
    state.set_key_state(0xE1, true);        // Left Shift
    assert(state.get_modifiers() == 0x03);  // Ctrl + Shift
    report = state.get_report();
    assert(report[0] == 0x03);

    // Release Left Ctrl
    state.set_key_state(0xE0, false);
    assert(state.get_modifiers() == 0x02);  // Only Shift
    report = state.get_report();
    assert(report[0] == 0x02);

    // Clear all
    state.clear();
    assert(state.get_modifiers() == 0);
    assert(state.get_pressed_key_count() == 0);

    std::cout << "PASSED\n";
}

void test_regular_keys() {
    std::cout << "Testing regular key handling... ";

    hid::KeyboardState state;

    // Press 'A' key (HID code 0x04)
    state.set_key_state(0x04, true);
    assert(state.get_pressed_key_count() == 1);

    auto report = state.get_report();
    assert(report[0] == 0);     // No modifiers
    assert(report[1] == 0);     // Reserved
    assert(report[2] == 0x04);  // 'A' key
    assert(report[3] == 0);     // Empty

    // Press 'B' key (HID code 0x05)
    state.set_key_state(0x05, true);
    assert(state.get_pressed_key_count() == 2);

    report = state.get_report();
    assert(report[2] == 0x04);  // 'A' key
    assert(report[3] == 0x05);  // 'B' key

    // Release 'A' key
    state.set_key_state(0x04, false);
    assert(state.get_pressed_key_count() == 1);

    report = state.get_report();
    assert(report[2] == 0x05);  // Only 'B' key
    assert(report[3] == 0);     // Empty

    std::cout << "PASSED\n";
}

void test_key_rollover() {
    std::cout << "Testing 6-key rollover limit... ";

    hid::KeyboardState state;

    // Press 7 keys (more than 6-key rollover limit)
    for (uint8_t i = 0x04; i <= 0x0A; ++i) {  // A through G
        state.set_key_state(i, true);
    }

    assert(state.get_pressed_key_count() == 7);  // All keys recorded

    auto report = state.get_report();
    assert(report[0] == 0);  // No modifiers
    assert(report[1] == 0);  // Reserved

    // Should only have 6 keys in report (bytes 2-7)
    int non_zero_count = 0;
    for (int i = 2; i < 8; ++i) {
        if (report[i] != 0) {
            non_zero_count++;
        }
    }
    assert(non_zero_count == 6);  // Only 6 keys in report

    std::cout << "PASSED\n";
}

void test_combined_modifiers_and_keys() {
    std::cout << "Testing combined modifiers and regular keys... ";

    hid::KeyboardState state;

    // Press Ctrl+Shift+A
    state.set_key_state(0xE0, true);  // Left Ctrl
    state.set_key_state(0xE1, true);  // Left Shift
    state.set_key_state(0x04, true);  // 'A' key

    auto report = state.get_report();
    assert(report[0] == 0x03);  // Ctrl + Shift modifiers
    assert(report[1] == 0);     // Reserved
    assert(report[2] == 0x04);  // 'A' key

    assert(state.get_modifiers() == 0x03);
    assert(state.get_pressed_key_count() == 1);

    std::cout << "PASSED\n";
}

void test_linux_key_mapping() {
    std::cout << "Testing Linux key event application... ";

    hid::KeyboardState state;

    // Test KEY_A (Linux code 30)
    bool handled = hid::apply_key_event(state, 30, 1);  // Press
    assert(handled);
    assert(state.get_pressed_key_count() == 1);

    auto report = state.get_report();
    assert(report[2] == 0x04);  // HID code for 'A'

    // Test key release
    handled = hid::apply_key_event(state, 30, 0);  // Release
    assert(handled);
    assert(state.get_pressed_key_count() == 0);

    // Test KEY_LEFTCTRL (Linux code 29)
    handled = hid::apply_key_event(state, 29, 1);  // Press
    assert(handled);
    assert(state.get_modifiers() == 0x01);  // Left Ctrl

    // Test auto-repeat (value 2)
    handled = hid::apply_key_event(state, 30, 2);  // Auto-repeat 'A'
    assert(handled);
    assert(state.get_pressed_key_count() == 1);

    std::cout << "PASSED\n";
}

void test_state_clear() {
    std::cout << "Testing state clear functionality... ";

    hid::KeyboardState state;

    // Press multiple keys and modifiers
    state.set_key_state(0xE0, true);  // Left Ctrl
    state.set_key_state(0xE1, true);  // Left Shift
    state.set_key_state(0x04, true);  // 'A' key
    state.set_key_state(0x05, true);  // 'B' key

    assert(state.get_modifiers() != 0);
    assert(state.get_pressed_key_count() != 0);

    // Clear all
    state.clear();

    assert(state.get_modifiers() == 0);
    assert(state.get_pressed_key_count() == 0);

    auto report = state.get_report();
    for (int i = 0; i < 8; ++i) {
        assert(report[i] == 0);
    }

    std::cout << "PASSED\n";
}

void test_dirty_flag() {
    std::cout << "Testing dirty flag functionality... ";

    hid::KeyboardState state;

    // Initial state should be dirty (first report)
    assert(state.is_dirty());

    // Getting report should clear dirty flag
    auto report = state.get_report();
    assert(!state.is_dirty());

    // Changing state should set dirty flag
    state.set_key_state(0x04, true);
    assert(state.is_dirty());

    // Getting report should clear dirty flag again
    report = state.get_report();
    assert(!state.is_dirty());

    std::cout << "PASSED\n";
}

}  // namespace

int main() {
    std::cout << "=== HID Keycodes Unit Tests ===\n";

    try {
        test_keyboard_state_basic();
        test_modifier_keys();
        test_regular_keys();
        test_key_rollover();
        test_combined_modifiers_and_keys();
        test_linux_key_mapping();
        test_state_clear();
        test_dirty_flag();

        std::cout << "\n=== All HID keycodes tests completed ===\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << "\n";
        return 1;
    }
}
