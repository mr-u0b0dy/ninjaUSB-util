# Development Guide

This guide provides comprehensive information for developers contributing to ninjaUSB-util.

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
   ctest
   ```

4. **Test your changes** before submitting

## Project Structure

```
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
│   ├── test_device_manager.cpp
│   └── test_args.cpp
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

Thank you for contributing to ninjaUSB-util! Every contribution, no matter how small, helps make this project better for everyone.
