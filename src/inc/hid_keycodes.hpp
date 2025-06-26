/* ---------------------------------------------------------------------------
 *  Linux KEY_*  →  USB HID Usage ID
 *  – Complete table for a PC-105 keyboard
 *  – All codes taken from USB HID Usage Tables v1.4, §10 (Keyboard/Keypad)
 *  – Any KEY_* not present here will simply be ignored by handle_key_event()
 * ---------------------------------------------------------------------------
 */

#include <cstdint>
#include <linux/input-event-codes.h> // Linux KEY_* codes
#include <unordered_map>

const std::unordered_map<int, uint8_t> linuxToHID = {
    /* --- Alphabet -------------------------------------------------------- */
    {KEY_A, 0x04}, {KEY_B, 0x05}, {KEY_C, 0x06}, {KEY_D, 0x07},
    {KEY_E, 0x08}, {KEY_F, 0x09}, {KEY_G, 0x0A}, {KEY_H, 0x0B},
    {KEY_I, 0x0C}, {KEY_J, 0x0D}, {KEY_K, 0x0E}, {KEY_L, 0x0F},
    {KEY_M, 0x10}, {KEY_N, 0x11}, {KEY_O, 0x12}, {KEY_P, 0x13},
    {KEY_Q, 0x14}, {KEY_R, 0x15}, {KEY_S, 0x16}, {KEY_T, 0x17},
    {KEY_U, 0x18}, {KEY_V, 0x19}, {KEY_W, 0x1A}, {KEY_X, 0x1B},
    {KEY_Y, 0x1C}, {KEY_Z, 0x1D},

    /* --- Number row ------------------------------------------------------ */
    {KEY_1, 0x1E}, {KEY_2, 0x1F}, {KEY_3, 0x20}, {KEY_4, 0x21},
    {KEY_5, 0x22}, {KEY_6, 0x23}, {KEY_7, 0x24}, {KEY_8, 0x25},
    {KEY_9, 0x26}, {KEY_0, 0x27},

    /* --- Punctuation & symbols ------------------------------------------ */
    {KEY_ENTER,        0x28},
    {KEY_ESC,          0x29},
    {KEY_BACKSPACE,    0x2A},
    {KEY_TAB,          0x2B},
    {KEY_SPACE,        0x2C},
    {KEY_MINUS,        0x2D}, /* - _ */
    {KEY_EQUAL,        0x2E}, /* = + */
    {KEY_LEFTBRACE,    0x2F}, /* [ { */
    {KEY_RIGHTBRACE,   0x30}, /* ] } */
    {KEY_BACKSLASH,    0x31}, /* \ | */
    {KEY_SEMICOLON,    0x33}, /* ; : */
    {KEY_APOSTROPHE,   0x34}, /* ' " */
    {KEY_GRAVE,        0x35}, /* ` ~ */
    {KEY_COMMA,        0x36}, /* , < */
    {KEY_DOT,          0x37}, /* . > */
    {KEY_SLASH,        0x38}, /* / ? */
    {KEY_CAPSLOCK,     0x39},

    /* --- Function row ---------------------------------------------------- */
    {KEY_F1,  0x3A}, {KEY_F2,  0x3B}, {KEY_F3,  0x3C}, {KEY_F4,  0x3D},
    {KEY_F5,  0x3E}, {KEY_F6,  0x3F}, {KEY_F7,  0x40}, {KEY_F8,  0x41},
    {KEY_F9,  0x42}, {KEY_F10, 0x43}, {KEY_F11, 0x44}, {KEY_F12, 0x45},

    /* --- Print / Scroll / Pause ----------------------------------------- */
    {KEY_SYSRQ,   0x46}, /* PrintScreen */
    {KEY_SCROLLLOCK, 0x47},
    {KEY_PAUSE,   0x48}, /* Pause|Break */

    /* --- Insert / Delete / Home / End / Pg keys ------------------------- */
    {KEY_INSERT, 0x49}, {KEY_HOME,   0x4A}, {KEY_PAGEUP,   0x4B},
    {KEY_DELETE, 0x4C}, {KEY_END,    0x4D}, {KEY_PAGEDOWN, 0x4E},

    /* --- Arrow cluster -------------------------------------------------- */
    {KEY_RIGHT, 0x4F}, {KEY_LEFT, 0x50},
    {KEY_DOWN,  0x51}, {KEY_UP,   0x52},

    /* --- KP (numeric keypad) ------------------------------------------- */
    {KEY_NUMLOCK, 0x53},
    {KEY_KPSLASH, 0x54}, {KEY_KPASTERISK, 0x55}, {KEY_KPMINUS,   0x56},
    {KEY_KPPLUS,  0x57}, {KEY_KPENTER,    0x58},
    {KEY_KP1, 0x59}, {KEY_KP2, 0x5A}, {KEY_KP3, 0x5B},
    {KEY_KP4, 0x5C}, {KEY_KP5, 0x5D}, {KEY_KP6, 0x5E},
    {KEY_KP7, 0x5F}, {KEY_KP8, 0x60}, {KEY_KP9, 0x61},
    {KEY_KP0, 0x62}, {KEY_KPDOT, 0x63},
    {KEY_KPEQUAL, 0x67},

    /* --- International & ISO keys -------------------------------------- */
    {KEY_102ND,       0x64}, /* Non-US \ | (ISO) */
    {KEY_RO,          0x87},
    // {KEY_KANA,        0x88},
    // {KEY_JP_YEN, 0x89},
    {KEY_HENKAN,      0x8A},
    {KEY_MUHENKAN, 0x8B},
    {KEY_KATAKANAHIRAGANA, 0x90},
    // {KEY_JP_BACKSLASH, 0x92},

    /* --- Modifier keys -------------------------------------------------- */
    {KEY_LEFTCTRL,   0xE0}, {KEY_LEFTSHIFT, 0xE1},
    {KEY_LEFTALT,    0xE2}, {KEY_LEFTMETA,  0xE3},
    {KEY_RIGHTCTRL,  0xE4}, {KEY_RIGHTSHIFT,0xE5},
    {KEY_RIGHTALT,   0xE6}, {KEY_RIGHTMETA, 0xE7},  /* GUI / Win */

    /* --- Media/system keys (HID consumer usage page uses 0xE8.. ) ------- */
    {KEY_MENU, 0x65},          /* Application/Context Menu */
    {KEY_POWER, 0x66},         /* System Power   (optional) */
    {KEY_SLEEP, 0x68},         /* System Sleep   (optional) */
    {KEY_WAKEUP, 0x69},        /* System Wake    (optional) */
};

const std::unordered_map<int, uint16_t> linuxToConsumerHID = {
    {KEY_VOLUMEUP,   0x00E9},
    {KEY_VOLUMEDOWN, 0x00EA},
    {KEY_MUTE,       0x00E2},
    {KEY_PLAYPAUSE,  0x00CD},
    {KEY_NEXTSONG,   0x00B5},
    {KEY_PREVIOUSSONG, 0x00B6},
    {KEY_STOPCD,     0x00B7},
    {KEY_EJECTCD,    0x00B8},
    {KEY_BRIGHTNESSUP,   0x006F},
    {KEY_BRIGHTNESSDOWN, 0x0070}
};

