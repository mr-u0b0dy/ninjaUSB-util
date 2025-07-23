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

**New Streamlined CI/CD Pipeline**: We've consolidated our workflows from 7 to 3 files
for better efficiency. The main CI pipeline now includes integrated quality
checks, conditional security scanning, and smart caching. See
**[Pipeline Documentation](doc/PIPELINE.md)** for details.

## 📚 Detailed Documentation

For comprehensive development guidelines, see
**[doc/CONTRIBUTING.md](doc/CONTRIBUTING.md)** which includes:

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
```

### Code Formatting

We use `clang-format` to maintain consistent code style. A formatting script is provided for convenience:

```bash
# Check code formatting (what CI will do)
./scripts/format-code.sh --check

# Apply formatting to all C++ files
./scripts/format-code.sh

# Manual formatting commands
find src tests -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i
clang-format --dry-run --Werror src/*.cpp src/inc/*.hpp
```

**Optional Pre-commit Hook**: To automatically check formatting before commits:

```bash
cp scripts/pre-commit-hook.sh .git/hooks/pre-commit
chmod +x .git/hooks/pre-commit
```

### Individual Quality Checks

```bash
# Code formatting check (CI will run comprehensive checks)
./scripts/format-code.sh --check

# Documentation linting
markdownlint-cli2 *.md doc/*.md

# Memory check
cd build && valgrind --tool=memcheck --leak-check=full ./test_device_manager
```

## Getting Help

- 📚 Check existing documentation first
- 🔍 Search [existing issues](https://github.com/your-username/ninjaUSB-util/issues)
- 💬 Ask questions in [GitHub Discussions](https://github.com/your-username/ninjaUSB-util/discussions)

Thank you for contributing! 🚀
