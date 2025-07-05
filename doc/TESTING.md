# Testing Guide

This guide covers testing procedures and requirements for ninjaUSB-util.

## 🔄 CI/CD Pipeline Testing - Strict Enforcement

Our comprehensive CI/CD pipeline automatically runs extensive testing with
**strict quality enforcement**. For complete pipeline details, see
[PIPELINE.md](PIPELINE.md).

### ⚠️ Strict Testing Requirements

- **All tests must pass** - no test failures are tolerated
- **Zero memory leaks** - Valgrind validation is strictly enforced
- **100% build success** - all platforms must compile successfully
- **Complete coverage** - all quality gates must pass

### Automated Testing in CI

The pipeline includes strict validation for:

- **Quick Validation**: File encoding and basic structure validation - **FAIL ON VIOLATIONS**
- **Quality Compliance**: License, code quality, and documentation checks - **FAIL ON WARNINGS**
- **Build Matrix**: Multi-platform builds (Ubuntu 22.04, 24.04) - **FAIL ON BUILD ERRORS**
- **Unit Tests**: Comprehensive test execution with CTest - **FAIL ON TEST FAILURES**
- **Static Analysis**: cppcheck and clang-tidy analysis - **FAIL ON CODE ISSUES**
- **Memory Testing**: Valgrind leak detection - **FAIL ON MEMORY LEAKS**
- **Performance Testing**: Memory and timing validation (on `[perf]` commits) - **FAIL ON REGRESSIONS**

## Unit Tests

The project includes comprehensive unit tests for core components:

```bash
# Build with tests
cmake .. -DBUILD_TESTS=ON
make

# Run all tests
ctest

# Run specific tests
./test_device_manager      # Device management tests
./test_args               # Argument parsing tests
./test_hid_keycodes       # HID keyboard mapping tests
./test_logger             # Logging system tests
./test_hotkey_detector    # Exit hotkey detection tests
./test_signal_handler     # Signal handling tests
./test_make_report_writer # BLE report writing tests
```

### Test Suite Overview

Our comprehensive test suite covers:

- **Device Management** (`test_device_manager`): Keyboard detection, hot-plug events, error handling
- **Argument Parsing** (`test_args`): All CLI options, validation, edge cases
- **HID Processing** (`test_hid_keycodes`): Key mapping, modifier handling, report generation
- **Logging System** (`test_logger`): Log levels, formatting, concurrent access
- **Hotkey Detection** (`test_hotkey_detector`): Exit combination detection (Alt+Ctrl+H)
- **Signal Handling** (`test_signal_handler`): SIGINT filtering, SIGTERM handling
- **BLE Communication** (`test_make_report_writer`): HID report transmission with mock BLE

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

Our comprehensive test suite provides excellent coverage for:

- **Device Manager**: Keyboard detection, hot-plug support, error handling, device validation
- **Argument Parser**: All CLI options, validation, error cases, help/version display
- **HID Processing**: Keycode conversion, modifier handling, report generation, state management
- **Logger**: Log levels, message formatting, concurrent access, timestamp functionality
- **Hotkey Detection**: Exit key combination (Alt+Ctrl+H), modifier tracking, key release handling
- **Signal Handling**: SIGINT filtering (Ctrl+C disabled), SIGTERM graceful shutdown, signal safety
- **BLE Communication**: HID report transmission, error handling, service validation

### Architecture Coverage

- **Core Logic**: 100% coverage of testable business logic components
- **Input Processing**: Complete HID keyboard state management and report generation
- **System Integration**: Signal handling and device management with proper error paths
- **Communication**: BLE report writing with comprehensive edge case handling

### Integration Test Areas

Areas requiring system/integration testing (beyond unit test scope):

- **End-to-End BLE**: Real BLE device communication (requires hardware)
- **Qt Event Loop**: Main application event processing (requires Qt test framework)
- **Hardware Integration**: Physical keyboard interaction (requires real devices)
- **System Permissions**: Root access and device permissions (system-dependent)

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
