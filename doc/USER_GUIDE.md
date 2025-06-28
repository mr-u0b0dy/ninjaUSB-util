# User Guide

Complete guide for using ninjaUSB-util.

## What is ninjaUSB-util?

A Linux utility that bridges USB keyboard input to Bluetooth Low Energy (BLE)
devices, enabling you to use your physical keyboard to control BLE devices that
accept HID keyboard input.

## How It Works

1. **Keyboard Monitoring**: Uses `udev` and `libevdev` to monitor Linux input
   devices (`/dev/input/eventX`)
2. **Input Processing**: Converts Linux keyboard events to USB HID usage codes
3. **BLE Communication**: Uses Qt6 Bluetooth to discover and connect to BLE devices
4. **Report Transmission**: Sends HID keyboard reports to writable BLE characteristics

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

# Build with tests (optional)
cmake .. -DBUILD_TESTS=ON
make

# Run tests
ctest

# Or use ninja if available
cmake -G Ninja ..
ninja
```

## Command Line Options

```bash
# Show help
./ninja_util --help

# Show version information
./ninja_util --version

# Run with verbose logging
./ninja_util -V

# List available BLE devices
./ninja_util --list-devices

# Connect to specific device
./ninja_util --target AA:BB:CC:DD:EE:FF

# Adjust scan timeout (default: 10 seconds)
./ninja_util --scan-timeout 5000

# Set log level
./ninja_util --log-level debug

# Combine options
./ninja_util -V --scan-timeout 5000 --log-level debug
```

## Basic Usage

1. **Run the utility** (requires root privileges for keyboard access):

   ```bash
   sudo ./ninja_util
   ```

2. **Device Discovery**: The program will scan for BLE devices for 10 seconds and
   display a list

3. **Select Target Device**: Choose the device number you want to connect to

4. **Start Typing**: Once connected, keyboard input will be forwarded to the
   selected BLE device

5. **Exit**: Press Ctrl+C to quit. TODO: will change to quit hot key.

## Supported Keys

The utility supports a comprehensive set of keyboard keys including:

- **Alphabet keys** (A-Z)
- **Number row** (0-9)
- **Function keys** (F1-F12)
- **Modifier keys** (Ctrl, Alt, Shift, Meta/Windows)
- **Special keys** (Enter, Backspace, Tab, Space, Arrow keys)
- **Punctuation and symbols**

See `src/inc/hid_keycodes.hpp` for the complete mapping of Linux key codes to USB
HID usage IDs.

## Architecture

```text
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   USB Keyboard  │ => │  ninjaUSB-util  │ => │   BLE Device    │
│                 │    │                 │    │                 │
│ /dev/input/     │    │ • Device Mgmt   │    │ HID Keyboard    │
│ eventX          │    │ • udev monitor  │    │ Reports         │
│                 │    │ • libevdev      │    │                 │
│                 │    │ • Qt6 Bluetooth │    │                 │
│                 │    │ • Arg parsing   │    │                 │
│                 │    │ • Logging       │    │                 │
└─────────────────┘    └─────────────────┘    └─────────────────┘
```

### Key Components

- **Device Manager**: Handles keyboard detection and hot-plug events
- **Argument Parser**: Modern CLI with comprehensive options
- **Logger**: Configurable logging with multiple levels
- **HID Processing**: Converts Linux input events to HID reports
- **BLE Communication**: Manages device discovery and connections
- **Version System**: Centralized version management from VERSION file

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
