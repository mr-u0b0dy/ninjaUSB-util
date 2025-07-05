/**
 * @file test_hotkey_detector.cpp
 * @brief Unit tests for exit hotkey detection functionality
 * @author Dharun A P
 * @license SPDX-License-Identifier: Apache-2.0
 * @copyright SPDX-FileCopyrightText: 2025 Dharun A P
 */

#include <cassert>
#include <iostream>
#include <string>

// Since ExitHotkeyDetector is defined in main.cpp, we need to extract it
// For testing purposes, let's create a standalone version
#include <linux/input-event-codes.h>

class ExitHotkeyDetector {
private:
    bool ctrl_pressed_ = false;
    bool alt_pressed_ = false;
    bool h_pressed_ = false;
    
public:
    bool process_key_event(int linux_code, int value) {
        bool is_press = (value == 1);
        bool is_release = (value == 0);
        
        // Track modifier key states
        switch (linux_code) {
            case KEY_LEFTCTRL:
            case KEY_RIGHTCTRL:
                if (is_press) ctrl_pressed_ = true;
                else if (is_release) ctrl_pressed_ = false;
                break;
                
            case KEY_LEFTALT:
            case KEY_RIGHTALT:
                if (is_press) alt_pressed_ = true;
                else if (is_release) alt_pressed_ = false;
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
        }
        
        return false;
    }
    
    std::string get_state_description() const {
        return "Ctrl: " + std::string(ctrl_pressed_ ? "ON" : "OFF") + 
               ", Alt: " + std::string(alt_pressed_ ? "ON" : "OFF") + 
               ", H: " + std::string(h_pressed_ ? "ON" : "OFF");
    }
    
    // Test helper methods
    bool is_ctrl_pressed() const { return ctrl_pressed_; }
    bool is_alt_pressed() const { return alt_pressed_; }
    bool is_h_pressed() const { return h_pressed_; }
};

namespace {

void test_individual_keys() {
    std::cout << "Testing individual key tracking... ";
    
    ExitHotkeyDetector detector;
    
    // Test Ctrl key
    bool exit_triggered = detector.process_key_event(KEY_LEFTCTRL, 1); // Press
    assert(!exit_triggered);
    assert(detector.is_ctrl_pressed());
    
    exit_triggered = detector.process_key_event(KEY_LEFTCTRL, 0); // Release
    assert(!exit_triggered);
    assert(!detector.is_ctrl_pressed());
    
    // Test Alt key
    exit_triggered = detector.process_key_event(KEY_LEFTALT, 1); // Press
    assert(!exit_triggered);
    assert(detector.is_alt_pressed());
    
    exit_triggered = detector.process_key_event(KEY_LEFTALT, 0); // Release
    assert(!exit_triggered);
    assert(!detector.is_alt_pressed());
    
    // Test H key
    exit_triggered = detector.process_key_event(KEY_H, 1); // Press
    assert(!exit_triggered);
    assert(detector.is_h_pressed());
    
    exit_triggered = detector.process_key_event(KEY_H, 0); // Release
    assert(!exit_triggered);
    assert(!detector.is_h_pressed());
    
    std::cout << "PASSED\n";
}

void test_partial_combinations() {
    std::cout << "Testing partial key combinations... ";
    
    ExitHotkeyDetector detector;
    
    // Test Ctrl+Alt (without H)
    detector.process_key_event(KEY_LEFTCTRL, 1);
    detector.process_key_event(KEY_LEFTALT, 1);
    bool exit_triggered = detector.process_key_event(KEY_A, 1); // Press 'A' instead of 'H'
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
    std::cout << "Testing full Alt+Ctrl+H combination... ";
    
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
    std::cout << "Testing different key press orders... ";
    
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
    exit_triggered = detector2.process_key_event(KEY_H, 0); // Release H
    assert(!exit_triggered);
    exit_triggered = detector2.process_key_event(KEY_H, 1); // Press H again
    assert(exit_triggered);
    
    std::cout << "PASSED\n";
}

void test_right_side_modifiers() {
    std::cout << "Testing right-side modifier keys... ";
    
    ExitHotkeyDetector detector;
    
    // Test with right Ctrl and right Alt
    detector.process_key_event(KEY_RIGHTCTRL, 1);
    detector.process_key_event(KEY_RIGHTALT, 1);
    bool exit_triggered = detector.process_key_event(KEY_H, 1);
    assert(exit_triggered);
    
    std::cout << "PASSED\n";
}

void test_mixed_modifiers() {
    std::cout << "Testing mixed left/right modifiers... ";
    
    ExitHotkeyDetector detector;
    
    // Test with left Ctrl and right Alt
    detector.process_key_event(KEY_LEFTCTRL, 1);
    detector.process_key_event(KEY_RIGHTALT, 1);
    bool exit_triggered = detector.process_key_event(KEY_H, 1);
    assert(exit_triggered);
    
    std::cout << "PASSED\n";
}

void test_key_release_behavior() {
    std::cout << "Testing key release behavior... ";
    
    ExitHotkeyDetector detector;
    
    // Set up the combination
    detector.process_key_event(KEY_LEFTCTRL, 1);
    detector.process_key_event(KEY_LEFTALT, 1);
    detector.process_key_event(KEY_H, 1);
    
    // Release one key
    detector.process_key_event(KEY_LEFTCTRL, 0);
    
    // Pressing H again should not trigger (Ctrl is released)
    bool exit_triggered = detector.process_key_event(KEY_H, 0); // Release
    exit_triggered = detector.process_key_event(KEY_H, 1); // Press again
    assert(!exit_triggered);
    
    std::cout << "PASSED\n";
}

void test_state_description() {
    std::cout << "Testing state description... ";
    
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
    std::cout << "Testing key repeat behavior... ";
    
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

} // namespace

int main() {
    std::cout << "=== Exit Hotkey Detector Unit Tests ===\n";
    
    try {
        test_individual_keys();
        test_partial_combinations();
        test_full_combination();
        test_different_key_orders();
        test_right_side_modifiers();
        test_mixed_modifiers();
        test_key_release_behavior();
        test_state_description();
        test_key_repeat();
        
        std::cout << "\n=== All hotkey detector tests completed ===\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << "\n";
        return 1;
    }
}
