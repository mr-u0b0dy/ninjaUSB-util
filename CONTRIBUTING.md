# Contributing to ninjaUSB-util

We welcome contributions! Here's how to get started quickly:

## Quick Start

1. **Fork** the repository on GitHub
2. **Clone** your fork locally
3. **Create a branch**: `git checkout -b feature/your-feature-name`
4. **Build with tests**: `cmake .. -DBUILD_TESTS=ON && make`
5. **Run tests**: `ctest`
6. **Make your changes** and test them
7. **Submit a pull request**

## Code Style

- Use C++17 features appropriately
- Follow existing code formatting
- Add clear comments and proper error handling
- Use the logger utility instead of std::cout
- Add SPDX license headers to new files
- Add unit tests for new functionality

## Testing

Run all tests before submitting:

```bash
# Build with tests
cmake .. -DBUILD_TESTS=ON
make

# Run all tests
ctest
```

## Documentation

For detailed information, see:

- **[Development Guide](doc/DEVELOPMENT.md)** - Comprehensive development setup
  and guidelines
- **[Testing Guide](doc/TESTING.md)** - Detailed testing procedures and checklist
- **[Versioning Guide](doc/VERSIONING.md)** - Version management system

## Questions?

- Open an issue for discussion
- Start with small bug fixes or documentation improvements
- Ask for guidance on larger features before implementing

Thank you for contributing!
