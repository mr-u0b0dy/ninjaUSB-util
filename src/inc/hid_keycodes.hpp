#pragma once
//  ---------------------------------------------------------------------------
//  HID Keycode Maps & Helpers
//  ---------------------------------------------------------------------------
//  * Linux KEY_* -> USB HID Usage IDs (Keyboard/Keypad page 0x07)
//  * Linux KEY_* -> Consumer‑Control Usage IDs (Consumer page 0x0C)
//  * Lightweight helper utilities to build 8‑byte keyboard reports and
//    2‑byte consumer‑control reports.
//  ---------------------------------------------------------------------------

#include <linux/input-event-codes.h>  // Linux KEY_* codes
#include <array>
#include <cstdint>
#include <set>
#include <unordered_map>

namespace hid {

// ---------------------------------------------------------------------------
//  Static lookup tables
// ---------------------------------------------------------------------------
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
    // {KEY_102ND, 0x64}, {KEY_RO, 0x87}, {KEY_KANA, 0x88},
    // {KEY_JP_YEN, 0x89}, {KEY_HENKAN, 0x8A}, {KEY_MUHENKAN, 0x8B},
    // {KEY_KATAKANAHIRAGANA, 0x90}, {KEY_JP_BACKSLASH, 0x92},

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
};

// ---------------------------------------------------------------------------
//  Helper utilities
// ---------------------------------------------------------------------------
inline constexpr bool is_modifier(std::uint8_t hid_code) {
    return hid_code >= 0xE0 && hid_code <= 0xE7;
}

struct KeyboardState {
    std::uint8_t modifiers{0};
    std::set<std::uint8_t> pressed;   // non‑modifier HID codes
    std::array<std::uint8_t, 8> report{};  // 8‑byte keyboard report

    void update() {
        report.fill(0);
        report[0] = modifiers;
        std::size_t idx = 2;
        for (auto key : pressed) {
            if (idx < report.size()) report[idx++] = key;
        }
    }
};

/**
 * Apply a Linux EV_KEY event to the keyboard state.
 * @return true  if handled as keyboard key
 *         false if not a keyboard key (caller can test consumer table instead)
 */
inline bool apply_key_event(KeyboardState &state, int linux_code, int value) {
    auto it = kKeyboardUsage.find(linux_code);
    if (it == kKeyboardUsage.end()) return false;  // Not a keyboard usage

    std::uint8_t hid_code = it->second;
    if (is_modifier(hid_code)) {
        std::uint8_t bit = 1u << (hid_code - 0xE0);
        if (value == 1)
            state.modifiers |= bit;
        else if (value == 0)
            state.modifiers &= ~bit;
    } else {
        if (value == 1)
            state.pressed.insert(hid_code);
        else if (value == 0)
            state.pressed.erase(hid_code);
    }
    state.update();
    return true;
}

/** Build a 2‑byte consumer‑control report (little‑endian). */
inline std::array<std::uint8_t, 2> consumer_report(int linux_code, int value) {
    std::uint16_t usage = 0;  // default: empty / key‑release
    if (value == 1) {
        auto it = kConsumerUsage.find(linux_code);
        if (it != kConsumerUsage.end()) usage = it->second;
    }
    return {static_cast<std::uint8_t>(usage & 0xFF), static_cast<std::uint8_t>((usage >> 8) & 0xFF)};
}

} // namespace hid
