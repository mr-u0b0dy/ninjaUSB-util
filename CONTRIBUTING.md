# Contributing to ninjaUSB-util

We welcome contributions! Here's how to get started quickly:

## Quick Start

1. **Fork** the repository on GitHub
2. **Clone** your fork locally
3. **Create a branch**: `git checkout -b feature/your-feature-name`
4. **Build with tests**: `mkdir build && cd build && cmake .. -DBUILD_TESTS=ON && ninja`
5. **Test your changes**: `ctest`
6. **Submit a Pull Request** with a clear description

## Development Guidelines

- Follow the existing code style (see `.clang-format`)
- Add tests for new functionality
- Update documentation as needed
- Ensure all CI checks pass

## 📚 Detailed Documentation

For comprehensive development guidelines, see **[doc/CONTRIBUTING.md](doc/CONTRIBUTING.md)** which includes:

- 🛠️ Complete development environment setup
- 🧪 Testing procedures and coverage guidelines  
- 📝 Coding standards and style requirements
- 🔍 Code quality tools and static analysis
- 🚀 Release process and versioning
- 🐛 Bug reporting templates
- ✨ Feature request guidelines

## Additional Resources

- **[Development Guide](doc/DEVELOPMENT.md)** - Technical development details
- **[Testing Guide](doc/TESTING.md)** - Testing procedures and guidelines
- **[Pipeline Documentation](doc/PIPELINE.md)** - CI/CD pipeline details
- **[User Guide](doc/USER_GUIDE.md)** - End-user documentation

## Quick Reference

### Building
```bash
mkdir build && cd build
cmake .. -DBUILD_TESTS=ON
make -j$(nproc)
```

### Testing
```bash
ctest --output-on-failure
```

### Development Tools
```bash
# Install development dependencies (Ubuntu/Debian)
sudo apt update && sudo apt install -y \
  cmake qt6-base-dev qt6-bluetooth-dev \
  libudev-dev libevdev-dev build-essential \
  clang-tidy cppcheck valgrind doxygen graphviz

# Install Node.js tools for documentation quality
npm install -g markdownlint-cli2 @mermaid-js/mermaid-cli markdown-link-check

# Individual quality checks (CI will run comprehensive checks)
clang-format --dry-run --Werror src/*.cpp src/inc/*.hpp
markdownlint-cli2 *.md doc/*.md

# Memory check
cd build && valgrind --tool=memcheck --leak-check=full ./test_device_manager
```

## Getting Help

- 📚 Check existing documentation first
- 🔍 Search [existing issues](https://github.com/your-username/ninjaUSB-util/issues)
- 💬 Ask questions in [GitHub Discussions](https://github.com/your-username/ninjaUSB-util/discussions)

Thank you for contributing! 🚀
