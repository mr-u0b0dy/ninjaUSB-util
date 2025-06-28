# ninjaUSB-util

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)

A Linux utility that bridges USB keyboard input to Bluetooth Low Energy (BLE) devices.

## Features

- **Real-time Keyboard Input Forwarding**: Captures keystrokes from USB keyboards and forwards them as HID reports over BLE
- **Hot-plug Support**: Automatically detects when keyboards are connected or disconnected
- **Multi-keyboard Support**: Can monitor multiple USB keyboards simultaneously
- **BLE Device Discovery**: Scans for and connects to BLE devices
- **HID Compliance**: Sends standard 8-byte HID keyboard reports compatible with most BLE devices
- **Modern Command-Line Interface**: Comprehensive argument parsing with help, version, and configuration options
- **Verbose Logging**: Detailed logging with configurable levels for debugging and monitoring
- **Automatic Versioning**: Centralized version management with build information

## Quick Start

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt install cmake qt6-base-dev qt6-bluetooth-dev libudev-dev libevdev-dev

# Build
mkdir build && cd build
cmake ..
make

# Run (requires sudo for keyboard access)
sudo ./ninja_util
```

## Usage

```bash
./ninja_util --help         # Show all options
./ninja_util --version      # Show version info
./ninja_util -V             # Run with verbose logging
./ninja_util --list-devices # List BLE devices
```

## Documentation

- **[User Guide](doc/USER_GUIDE.md)** - Installation, usage, and troubleshooting
- **[Contributing](CONTRIBUTING.md)** - Quick start for contributors
- **[Development](doc/DEVELOPMENT.md)** - Detailed development guide
- **[Testing](doc/TESTING.md)** - Testing procedures
- **[Versioning](doc/VERSIONING.md)** - Version management

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

## License

Licensed under the Apache License 2.0 - see [LICENSE](LICENSE) file.
