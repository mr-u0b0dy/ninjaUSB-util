# ninjaUSB-util

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)

A Linux utility that bridges USB keyboard input to Bluetooth Low Energy (BLE) devices, enabling you to use your physical keyboard to control BLE devices that accept HID keyboard input.

## Features

- **Real-time Keyboard Input Forwarding**: Captures keystrokes from USB keyboards and forwards them as HID reports over BLE
- **Hot-plug Support**: Automatically detects when keyboards are connected or disconnected
- **Multi-keyboard Support**: Can monitor multiple USB keyboards simultaneously
- **BLE Device Discovery**: Scans for and connects to BLE devices
- **HID Compliance**: Sends standard 8-byte HID keyboard reports compatible with most BLE devices

## How It Works

1. **Keyboard Monitoring**: Uses `udev` and `libevdev` to monitor Linux input devices (`/dev/input/eventX`)
2. **Input Processing**: Converts Linux keyboard events to USB HID usage codes
3. **BLE Communication**: Uses Qt6 Bluetooth to discover and connect to BLE devices
4. **Report Transmission**: Sends HID keyboard reports to writable BLE characteristics

## Dependencies

### Required Libraries

- **Qt6**: Core and Bluetooth modules
- **libudev**: For device detection and hot-plug support
- **libevdev**: For reading keyboard input events

### Build Tools

- CMake 3.20+
- C++17 compatible compiler
- pkg-config

## Installation

### Ubuntu/Debian

```bash
sudo apt install cmake build-essential pkg-config
sudo apt install qt6-base-dev qt6-bluetooth-dev
sudo apt install libudev-dev libevdev-dev
```

### Arch Linux

```bash
sudo pacman -S cmake base-devel pkg-config
sudo pacman -S qt6-base qt6-connectivity
sudo pacman -S libevdev systemd
```

## Building

```bash
# Clone the repository
git clone <repository-url>
cd ninjaUSB-util

# Create build directory
mkdir build && cd build

# Configure and build
cmake ..
make

# Or use ninja if available
cmake -G Ninja ..
ninja
```

## Usage

1. **Run the utility** (requires root privileges for keyboard access):

   ```bash
   sudo ./ninja_util
   ```

2. **Device Discovery**: The program will scan for BLE devices for 10 seconds and display a list

3. **Select Target Device**: Choose the device number you want to connect to

4. **Start Typing**: Once connected, keyboard input will be forwarded to the selected BLE device

5. **Exit**: Press Ctrl+C to quit. TODO: will change to quit hot key.

## Supported Keys

The utility supports a comprehensive set of keyboard keys including:

- **Alphabet keys** (A-Z)
- **Number row** (0-9)
- **Function keys** (F1-F12)
- **Modifier keys** (Ctrl, Alt, Shift, Meta/Windows)
- **Special keys** (Enter, Backspace, Tab, Space, Arrow keys)
- **Punctuation and symbols**

See `src/inc/hid_keycodes.hpp` for the complete mapping of Linux key codes to USB HID usage IDs.

## Architecture

```text
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   USB Keyboard  │ => │  ninjaUSB-util  │ => │   BLE Device    │
│                 │    │                 │    │                 │
│ /dev/input/     │    │ • udev monitor  │    │ HID Keyboard    │
│ eventX          │    │ • libevdev      │    │ Reports         │
│                 │    │ • Qt6 Bluetooth │    │                 │
└─────────────────┘    └─────────────────┘    └─────────────────┘
```

## Troubleshooting

### Permission Issues

- Run with `sudo` to access input devices
- Ensure your user is in the `input` group: `sudo usermod -a -G input $USER`

### No Keyboards Detected

- Check if keyboards appear in `/dev/input/`: `ls -la /dev/input/event*`
- Verify udev is working: `udevadm monitor --subsystem-match=input`

### BLE Connection Issues

- Ensure Bluetooth is enabled: `bluetoothctl power on`
- Check if the target device is in pairing/discoverable mode
- Verify Qt6 Bluetooth is properly installed

### Build Errors

- Ensure all dependencies are installed
- Check Qt6 installation: `qmake6 --version` or `cmake --find-package Qt6`

## License

This project is licensed under the Apache License 2.0 - see the [LICENSE](LICENSE) file for details.

## Contributing

We welcome contributions! Please see our [Contributing Guidelines](CONTRIBUTING.md) for details on how to get started, code style, and the development process.

## Related Projects

### ninjaUSB - BLE to USB HID Bridge Firmware

This utility complements the **ninjaUSB** project - a Zephyr-based firmware for the nRF52840 dongle that creates a BLE to USB HID bridge.

- **Repository**: [ninjaUSB](https://github.com/mr-u0b0dy/ninjaUSB/tree/dev)
- **Purpose**: nRF52840-based firmware that receives keystrokes via BLE and outputs them through USB HID
- **Architecture**: Reverse bridge - while ninjaUSB-util sends USB→BLE, ninjaUSB receives BLE→USB
- **Hardware**: nRF52840 dongle running Zephyr RTOS
- **Use Case**: Together, these projects create a complete wireless keyboard bridge solution

### Project Ecosystem

```text
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   USB Keyboard  │ => │ ninjaUSB-util   │ => │ ninjaUSB Device │ => │  Target Device  │
│                 │    │ (this project)  │    │ (nRF52840)      │    │  (via USB HID)  │
│ /dev/input/     │    │ • BLE Client    │    │ • BLE Server    │    │                 │
│ eventX          │    │ • Input capture │    │ • USB HID out   │    │                 │
└─────────────────┘    └─────────────────┘    └─────────────────┘    └─────────────────┘
```
