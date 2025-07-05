# Development Guide

This guide provides comprehensive information for developers contributing to ninjaUSB-util.

## 🚀 Quick Development Setup

1. **Install dependencies** (see Installation section in README)
2. **Build the project**:

   ```bash
   mkdir build && cd build
   cmake .. -DBUILD_TESTS=ON -DBUILD_DOCS=ON
   make -j$(nproc)
   ```

3. **Run tests** to verify everything works:

   ```bash
   ctest --output-on-failure
   ```

4. **Test your changes** against CI quality gates (see [PIPELINE.md](PIPELINE.md))

## 🔄 CI/CD Integration - Strict Quality Enforcement

Our development workflow is integrated with a comprehensive CI/CD pipeline with
**strict quality enforcement**. Before submitting changes:

### ⚠️ Important: Strict Quality Gates

- **All warnings are treated as errors** and will fail the pipeline
- **Zero tolerance policy** for quality violations
- **Complete validation** required for all files and changes
- **No manual overrides** permitted for quality violations

### Local Quality Checks

Run the same checks that CI will perform (all must pass with zero violations):

```bash
# Code formatting (must pass with zero violations)
find src tests -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i

# Static analysis (must pass with zero warnings/errors)
cppcheck --enable=all --std=c++17 --verbose \
  --suppress=missingIncludeSystem \
  --suppress=unusedFunction \
  src/ tests/

# Build with all options (must pass with zero warnings)
cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON -DBUILD_DOCS=ON \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
make -j$(nproc)

# Run tests (all tests must pass)
ctest --output-on-failure --verbose

# Memory checks (must pass with zero leaks)
valgrind --leak-check=full --show-leak-kinds=all ./ninja_util --help

# Documentation build
make docs
```

### Understanding CI Jobs

The CI pipeline runs these main jobs (see [PIPELINE.md](PIPELINE.md) for details):

1. **Quick Checks**: File validation and change detection
2. **Quality & Compliance**: License, markdown, C++ analysis, file validation, docs validation
3. **Build & Test**: Multi-platform matrix builds with comprehensive testing
4. **Performance Tests**: Memory and performance validation (conditional)
5. **Release**: Automated release preparation (tags only)

## Development Setup

1. **Install dependencies** (see Installation section in README)

2. **Build the project**:

   ```bash
   mkdir build && cd build
   cmake .. -DBUILD_TESTS=ON
   make
   ```

3. **Run tests** to verify everything works:

   ```bash
   ctest --output-on-failure
   ```

4. **Test your changes** comprehensively before submitting

## Testing & Quality Assurance

### Unit Testing

Our comprehensive test suite covers all core functionality:

```bash
# Build and run all tests
cd build
cmake .. -DBUILD_TESTS=ON
make -j$(nproc)
ctest --output-on-failure --verbose
```

### Test Coverage

The project maintains excellent test coverage:

- **Device Management**: Keyboard detection, hot-plug events, error handling
- **Argument Parsing**: All CLI options, validation, edge cases  
- **HID Processing**: Key mapping, modifier handling, report generation
- **Signal Handling**: SIGINT filtering, SIGTERM graceful shutdown
- **BLE Communication**: HID report transmission with comprehensive mocking
- **System Integration**: Exit hotkey detection, logging functionality

### Adding New Tests

When implementing new features:

1. **Create test file**: `tests/test_[feature_name].cpp`
2. **Add to CMakeLists.txt**: Include in `BUILD_TESTS` section
3. **Follow test patterns**: Use existing tests as templates
4. **Test edge cases**: Include error conditions and boundary values
5. **Verify integration**: Run full test suite to ensure no regressions

See [TESTING.md](TESTING.md) for detailed testing procedures.

## Project Structure

```text
ninjaUSB-util/
├── src/                    # Source code
│   ├── main.cpp           # Application entry point
│   ├── device_manager.cpp # Keyboard device management
│   ├── args.cpp           # Command-line argument parsing
│   ├── logger.cpp         # Logging utility
│   └── inc/               # Header files
│       ├── version.hpp.in # Version template (do not edit version.hpp)
│       ├── device_manager.hpp
│       ├── args.hpp
│       ├── logger.hpp
│       └── hid_keycodes.hpp
├── tests/                 # Unit tests
│   ├── test_device_manager.cpp    # Device management tests
│   ├── test_args.cpp              # Argument parsing tests
│   ├── test_hid_keycodes.cpp      # HID keyboard mapping tests
│   ├── test_logger.cpp            # Logging system tests
│   ├── test_hotkey_detector.cpp   # Exit hotkey detection tests
│   ├── test_signal_handler.cpp    # Signal handling tests
│   └── test_make_report_writer.cpp # BLE report writing tests
├── doc/                   # Documentation
│   ├── VERSIONING.md
│   ├── DEVELOPMENT.md
│   └── TESTING.md
├── VERSION                # Version source file
├── CMakeLists.txt         # Build configuration
└── build/                 # Build artifacts (auto-generated)
    └── include/
        └── version.hpp    # Generated version header
```

## Code Style Guidelines

- **C++ Standard**: Use C++17 features appropriately
- **Formatting**: Follow existing code style and indentation
- **Comments**: Add clear comments for complex logic
- **Error Handling**: Include proper error checking and cleanup
- **Logging**: Use the logger utility instead of std::cout for runtime messages
- **Testing**: Add unit tests for new functionality when applicable
- **SPDX Headers**: Add SPDX license identifiers to new files:

  ```cpp
  // SPDX-License-Identifier: Apache-2.0
  // SPDX-FileCopyrightText: 2025 Your Name
  ```

- **File Encoding**: Follow project encoding standards:
  - **Source files** (`.cpp`, `.hpp`): ASCII or UTF-8 encoding
  - **Documentation files** (`.md`): UTF-8 encouraged but not enforced
  - **Configuration files** (`.yml`, `.yaml`): UTF-8 recommended

### Version Management

- **Never edit version.hpp directly** - it's auto-generated
- **Update VERSION file** for version changes
- **Use version namespace** for accessing version information in code
- **Test version changes** by rebuilding and checking `--version` output

## Commit Guidelines

- **Commit Messages**: Use clear, descriptive commit messages
- **Scope**: Keep commits focused on a single change
- **Format**: Use conventional commit format when possible:

  ```text
  feat: add support for multimedia keys
  fix: resolve memory leak in keyboard cleanup
  docs: update installation instructions for Fedora
  ```

## Pull Request Process

1. **Update documentation** if your changes affect usage
2. **Test thoroughly** on your system
3. **Create a pull request** with:
   - Clear description of changes
   - Reference to any related issues
   - Testing performed
4. **Respond to feedback** during code review

## Issue Reporting

When reporting bugs, please include:

- **System Information**: OS, kernel version, Qt6 version
- **Hardware**: Keyboard and BLE device details
- **Steps to Reproduce**: Clear reproduction steps
- **Expected vs Actual Behavior**
- **Logs/Output**: Relevant error messages or debug output

### Bug Report Template

```markdown
**System Information:**

- OS: [e.g., Ubuntu 22.04]
- Kernel: [e.g., 5.15.0]
- Qt6 Version: [e.g., 6.5.0]

**Hardware:**

- Keyboard: [e.g., Logitech K380]
- BLE Device: [e.g., Samsung Smart TV]

**Description:**
[Clear description of the issue]

**Steps to Reproduce:**
1. [First step]
2. [Second step]
3. [...]

**Expected Behavior:**
[What you expected to happen]

**Actual Behavior:**
[What actually happened]

**Logs/Output:**
[Paste relevant error messages or debug output]
```

## Feature Requests

For feature requests, please describe:

- **Use Case**: Why is this feature needed?
- **Proposed Solution**: How should it work?
- **Alternatives**: Any alternative approaches considered
- **Implementation Ideas**: If you have technical suggestions

### Feature Request Template

```markdown
**Use Case:**
[Describe the problem or need this feature addresses]

**Proposed Solution:**
[Describe how you think this should work]

**Alternatives:**
[Any alternative solutions you've considered]

**Implementation Ideas:**
[If you have technical suggestions, include them here]

**Additional Context:**
[Any other context or screenshots about the feature request]
```

## Documentation

### Code Documentation

The project uses **Doxygen** for API documentation generation. All public
interfaces should be well-documented with comprehensive Doxygen comments.

#### Documentation Standards

- **All public classes** must have class-level documentation
- **All public methods** must have parameter and return value documentation
- **Use @brief, @param, @return** tags appropriately
- **Include usage examples** for complex interfaces
- **Document thread safety** and exception behavior
- **Explain design decisions** in architectural comments

#### Generating Documentation

```bash
# Install Doxygen
sudo apt install doxygen graphviz  # Ubuntu/Debian
sudo pacman -S doxygen graphviz     # Arch Linux

# Build documentation
mkdir build && cd build
cmake .. -DBUILD_DOCS=ON
make docs

# View documentation
firefox doc/api/html/index.html
```

#### Doxygen Configuration

The project uses `.doxygen` (note the dot prefix) as the Doxygen configuration
file. This file has been upgraded to the latest Doxygen format and includes
comprehensive configuration options and documentation.

The Doxygen configuration is integrated with CMake and automatically configures:

- **PROJECT_NAME**: Set from the CMake `PROJECT_NAME` variable for consistency
- **PROJECT_NUMBER**: Automatically populated from the `VERSION` file

Note: The PROJECT_NAME in documentation will show as "ninja_util" (the CMake
project name) rather than "ninjaUSB-util". This ensures consistency between the
build system and documentation.

To update the Doxygen configuration:

```bash
# Upgrade configuration file to latest format
doxygen -u .doxygen
```

#### Doxygen Theme

The project uses the **doxygen-awesome-css** theme for modern, responsive
documentation styling. The theme is automatically downloaded and integrated
using CMake's FetchContent module.

**Theme Features**:

- Modern, clean design
- Responsive layout for mobile devices
- Dark/light mode support
- Enhanced sidebar navigation
- Improved typography and spacing

**Theme Integration**:

- Theme files are downloaded during CMake configuration
- CSS files are automatically referenced in the Doxygen configuration
- No manual setup required - everything is handled by the build system

#### Documentation Structure

- **API Documentation**: Generated from Doxygen comments in source code
- **Architecture Guide**: High-level system design in `doc/ARCHITECTURE.md`
- **User Guide**: End-user documentation in `doc/USER_GUIDE.md`
- **Development Guide**: This document for contributors

#### Writing Good Documentation

```cpp
/**
 * @brief Brief description of the function
 * @param param1 Description of first parameter
 * @param param2 Description of second parameter
 * @return Description of return value
 * 
 * Detailed description of what the function does,
 * including any side effects, thread safety, and
 * usage examples.
 * 
 * @section Usage Usage Example
 * @code
 * auto result = my_function(value1, value2);
 * if (result) {
 *     // Handle success
 * }
 * @endcode
 * 
 * @note Important notes about usage
 * @warning Warnings about potential issues
 */
bool my_function(int param1, const std::string& param2);
```

### Version Documentation

- **Versioning Scheme**: Follows [Semantic Versioning](https://semver.org/)
- **Version Components**: MAJOR.MINOR.PATCH
- **Pre-release Versions**: Indicated by a hyphen and identifier (e.g., 1.0.0-alpha)
- **Build Metadata**: Optional, indicated by a plus sign (e.g., 1.0.0+20130313144700)

#### Updating Version Information

- Update the `VERSION` file with the new version number
- Commit the version change with a message like `bump version to 1.0.1`
- Tag the commit with the new version: `git tag -a v1.0.1 -m "Release version 1.0.1"`
- Push the tag to the repository: `git push origin v1.0.1`

## Code of Conduct

- Be respectful and constructive in discussions
- Welcome newcomers and help them get started
- Focus on technical merit in code reviews
- Report any unacceptable behavior to project maintainers

## Questions?

If you have questions about contributing, feel free to:

- Open an issue for discussion
- Start with documentation improvements or small bug fixes
- Ask for guidance on larger features before implementing

## Acknowledgments

Thank you for contributing to ninjaUSB-util! Every contribution, no matter how
small, helps make this project better for everyone.
