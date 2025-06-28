# Testing Guide

This guide covers testing procedures and requirements for ninjaUSB-util.

## Unit Tests

The project includes unit tests for core components:

```bash
# Build with tests
cmake .. -DBUILD_TESTS=ON
make

# Run all tests
ctest

# Run specific tests
./test_device_manager  # Device management tests
./test_args           # Argument parsing tests
```

## Manual Testing

### Basic Functionality

- **Keyboard Detection**: Test that keyboards are properly detected
- **BLE Connection**: Test connection to BLE devices
- **Key Forwarding**: Test that keystrokes are properly forwarded
- **Hot-plug Events**: Test connecting/disconnecting keyboards
- **Command-line Options**: Test all CLI arguments

### Command-Line Testing

Test all command-line options:

```bash
# Help and version
./ninja_util --help
./ninja_util --version

# Verbose logging
./ninja_util -V
./ninja_util --verbose

# Device listing
./ninja_util --list-devices

# Configuration options
./ninja_util --scan-timeout 5000
./ninja_util --log-level debug
./ninja_util --target AA:BB:CC:DD:EE:FF

# Combined options
./ninja_util -V --scan-timeout 5000 --log-level debug
```

### Hardware Testing

When possible, test with:

- **Different keyboards**: USB, wireless, different manufacturers
- **Different BLE devices**: Various device types that accept HID input
- **Multiple keyboards**: Test simultaneous multi-keyboard support
- **Hot-plug scenarios**: Connect/disconnect during operation

## Testing Checklist

Before submitting a PR, verify:

### Build and Compilation
- [ ] Code compiles without warnings
- [ ] All unit tests pass (`ctest`)
- [ ] No build errors with tests enabled

### Functionality
- [ ] Basic functionality works (keyboard detection, BLE connection)
- [ ] Command-line arguments work correctly
- [ ] Help and version display correctly
- [ ] Verbose logging works as expected
- [ ] Hot-plug events work correctly

### Quality
- [ ] No memory leaks or segmentation faults
- [ ] Error handling works as expected
- [ ] Logging output is appropriate
- [ ] Performance is acceptable

### Documentation
- [ ] Documentation updated if needed
- [ ] Code comments added for complex logic
- [ ] CHANGELOG updated for user-facing changes

## Test Coverage

### Current Test Coverage

- **Device Manager**: Basic functionality, error handling
- **Argument Parser**: All CLI options, validation, error cases
- **Logger**: (Manual testing - integrated in other components)
- **Version System**: (Manual testing via --version)

### Missing Test Coverage

Areas that could benefit from additional tests:

- **HID Processing**: Keycode conversion and report generation
- **BLE Communication**: Mock BLE device interactions
- **Integration Tests**: End-to-end keyboard to BLE forwarding

## Performance Testing

### Latency Testing

Test input latency by:

1. Using a high-frequency keyboard
2. Monitoring with verbose logging
3. Measuring time between key press and BLE transmission

### Resource Usage

Monitor resource usage during operation:

```bash
# Monitor CPU and memory usage
top -p $(pgrep ninja_util)

# Monitor system calls
strace -p $(pgrep ninja_util)

# Check for memory leaks
valgrind --leak-check=full ./ninja_util --list-devices
```

## Debugging

### Debug Build

For debugging, build with debug symbols:

```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
make
```

### Verbose Logging

Use verbose logging for debugging:

```bash
./ninja_util -V --log-level debug
```

### Common Issues

- **Permission denied**: Run with sudo or add user to input group
- **No keyboards found**: Check /dev/input/ permissions and udev rules
- **BLE connection fails**: Verify Bluetooth is enabled and device is discoverable
- **High CPU usage**: Check polling interval settings

## Automated Testing

### Continuous Integration

The project should be tested on:

- Multiple Linux distributions (Ubuntu, Fedora, Arch)
- Different Qt6 versions
- Various compiler versions (GCC, Clang)

### Test Data

For consistent testing, consider:

- Mock keyboard input events
- Simulated BLE device responses
- Predefined test scenarios

## Contributing Tests

When adding new features:

1. **Add unit tests** for new functionality
2. **Update existing tests** if behavior changes
3. **Add integration tests** for complex features
4. **Document test procedures** for manual testing
5. **Update this guide** with new testing requirements
