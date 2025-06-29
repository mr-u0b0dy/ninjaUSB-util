# Contributing to ninjaUSB-util

Thank you for your interest in contributing to ninjaUSB-util! This document provides guidelines for contributing to the project.

## 🚀 Quick Start for Contributors

1. **Fork the repository** on GitHub
2. **Clone your fork**: `git clone https://github.com/YOUR_USERNAME/ninjaUSB-util.git`
3. **Create a feature branch**: `git checkout -b feature/your-feature-name`
4. **Make your changes** following our coding standards
5. **Test your changes**: Run tests and ensure CI passes
6. **Submit a Pull Request** with a clear description

## 📋 Development Workflow

### Prerequisites

- Ubuntu 20.04+ (or compatible Linux distribution)
- CMake 3.20+
- Qt6 development libraries
- Git

### Setup Development Environment

```bash
# Install dependencies
sudo apt install cmake qt6-base-dev qt6-bluetooth-dev libudev-dev libevdev-dev

# Clone and build
git clone https://github.com/YOUR_USERNAME/ninjaUSB-util.git
cd ninjaUSB-util
mkdir build && cd build
cmake .. -DBUILD_TESTS=ON -DBUILD_DOCS=ON
make -j$(nproc)

# Run tests
ctest --output-on-failure
```

## 🔍 CI/CD Pipeline and Quality Checks - Strict Enforcement

Our comprehensive CI/CD pipeline ensures code quality and maintainability with **strict enforcement** - all warnings are treated as errors. For complete details, see [PIPELINE.md](PIPELINE.md).

### Pipeline Overview

The CI pipeline consists of these main jobs with **zero tolerance for quality violations**:

1. **Quick Checks** (< 30 seconds): Fast validation for immediate feedback - **FAIL ON VIOLATIONS**
2. **Quality & Compliance** (parallel): Comprehensive quality validation - **FAIL ON WARNINGS**
3. **Build & Test** (parallel): Multi-platform builds and testing - **FAIL ON ERRORS**
4. **Performance Tests** (conditional): Memory and performance validation - **FAIL ON LEAKS**
5. **Release** (tags only): Automated release preparation

### Quality Gates - Strict Enforcement

Your Pull Request must pass these **strictly enforced** checks:

- **License Compliance**: SPDX headers and license validation - **FAIL ON MISSING/INVALID**
- **Code Quality**: Formatting (clang-format), static analysis (cppcheck, clang-tidy) - **FAIL ON VIOLATIONS**
- **Documentation**: Coverage analysis and Doxygen validation - **FAIL ON INSUFFICIENT COVERAGE**
- **File Standards**: Encoding, structure, and markdown linting - **FAIL ON LINT ERRORS**
- **Build Success**: Multi-platform compilation (Ubuntu 22.04, 24.04) - **FAIL ON BUILD ERRORS**
- **Test Coverage**: Unit tests with CTest and memory leak detection - **FAIL ON TEST FAILURES**
- **Link Validation**: All markdown links must be valid - **FAIL ON BROKEN LINKS**
- **Mermaid Diagrams**: Syntax must be correct - **FAIL ON SYNTAX ERRORS**

### ⚠️ Important: Zero Tolerance Policy

- **All warnings are treated as errors** and will fail the pipeline
- **No manual overrides** are permitted for quality violations
- **Complete validation** is required for all files and changes
- **Immediate failure** occurs on first quality violation

### Running Quality Checks Locally

Before submitting your PR, run these checks locally:

```bash
# Code formatting
find src tests -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i

# Static analysis
cppcheck --enable=all --std=c++17 src/ tests/

# Build and test
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON -DBUILD_DOCS=ON
make -j$(nproc)
ctest --output-on-failure

# Memory checks (if available)
valgrind --leak-check=full ./ninja_util --help

# Documentation build
make docs
```

### Markdown and Documentation Standards

Our pipeline includes enhanced markdown validation:

- **Linting**: Automated markdownlint-cli2 with project-specific rules
- **Link Validation**: Broken internal link detection
- **Accessibility**: Image alt-text validation
- **Formatting**: Trailing whitespace and consistency checks

### Commit Message Guidelines

Use clear, descriptive commit messages:

```bash
# Good examples
git commit -m "Add BLE device discovery timeout configuration"
git commit -m "Fix memory leak in device manager cleanup"
git commit -m "Update PIPELINE.md with new quality gates"

# Include [perf] tag for performance-related commits
git commit -m "[perf] Optimize keyboard event processing loop"
```

## 🧪 Testing

### Running Tests

```bash
cd build
ctest --verbose
```

### Adding Tests

- Place test files in `tests/` directory
- Follow naming convention: `test_*.cpp`
- Add new tests to `CMakeLists.txt`

### Test Coverage

We aim for >80% test coverage. Run coverage analysis:

```bash
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="--coverage"
make
ctest
gcov src/*.cpp
```

## 📝 Coding Standards

### Code Style

- **Indentation**: 4 spaces (no tabs)
- **Line Length**: 100 characters maximum
- **Naming Convention**:
  - Variables/functions: `snake_case`
  - Classes: `PascalCase`
  - Constants: `UPPER_SNAKE_CASE`

### C++ Guidelines

- Use modern C++17 features
- Prefer RAII and smart pointers
- Include proper error handling
- Add comprehensive comments for public APIs

### Example Code Style

```cpp
// Good example
class DeviceManager {
private:
    std::vector<std::unique_ptr<KeyboardDevice>> keyboards_;
    
public:
    /**
     * @brief Adds a new keyboard device to the manager
     * @param device_path Path to the device file
     * @return true if device was added successfully
     */
    bool add_keyboard(const std::string& device_path);
};
```

Our pipeline includes:

- **Static Analysis**: clang-tidy, cppcheck
- **Memory Checking**: Valgrind
- **Security Scanning**: CodeQL, Trivy
- **Performance Testing**: Benchmarks and profiling

Ensure your code passes all checks:

```bash
# Static analysis
clang-tidy src/*.cpp -p build/

# Memory check
valgrind --tool=memcheck ./build/test_device_manager
```

## 📚 Documentation

### Adding Documentation

- Use Doxygen comments for all public APIs
- Update relevant `.md` files in `doc/` directory
- Include examples in docstrings

### Building Documentation

```bash
cd build
cmake .. -DBUILD_DOCS=ON
make docs
# Open build/doc/api/html/index.html
```

## 🐛 Bug Reports

When reporting bugs, please include:

- **Environment**: OS version, Qt version, dependencies
- **Steps to Reproduce**: Clear, numbered steps
- **Expected Behavior**: What should happen
- **Actual Behavior**: What actually happens
- **Logs**: Relevant error messages or logs

### Bug Report Template

```markdown
**Environment:**
- OS: Ubuntu 22.04
- Qt Version: 6.4.2
- CMake Version: 3.24.0

**Steps to Reproduce:**
1. Run `sudo ./ninja_util`
2. Connect USB keyboard
3. ...

**Expected:** Device should be detected
**Actual:** Application crashes with segfault

**Logs:**

[Include relevant log output]
```

## 🎯 Feature Requests

Before submitting feature requests:

1. **Search existing issues** to avoid duplicates
2. **Describe the use case** clearly
3. **Explain the benefit** to users
4. **Consider implementation complexity**

## 🔄 Pull Request Process

### Before Submitting

- [ ] Code compiles without warnings
- [ ] All tests pass locally
- [ ] Documentation is updated
- [ ] Commit messages are clear and descriptive
- [ ] Branch is up-to-date with main

### PR Description Template

```markdown
## Description
Brief description of changes

## Type of Change
- [ ] Bug fix
- [ ] New feature
- [ ] Documentation update
- [ ] Performance improvement
- [ ] Refactoring

## Testing
- [ ] Unit tests added/updated
- [ ] Manual testing performed
- [ ] CI pipeline passes

## Related Issues
Fixes #123
```

### Review Process

1. **Automated Checks**: CI must pass
2. **Code Review**: Maintainer review required
3. **Testing**: Manual testing by maintainers
4. **Merge**: Squash and merge preferred

## 🏷️ Versioning

We follow [Semantic Versioning](https://semver.org/):

- **MAJOR**: Breaking changes
- **MINOR**: New features (backward compatible)
- **PATCH**: Bug fixes

Version is managed in the `VERSION` file at project root.

## 📄 License

By contributing, you agree that your contributions will be licensed under the Apache 2.0 License.

## 🤝 Code of Conduct

### Our Standards

- **Be Respectful**: Treat everyone with respect
- **Be Collaborative**: Work together constructively
- **Be Professional**: Keep discussions focused and productive

### Enforcement

Violations may result in:

1. Warning
2. Temporary ban
3. Permanent ban

Report issues to project maintainers.

## 💬 Getting Help

- **Documentation**: Check `doc/` directory first
- **Discussions**: Use GitHub Discussions for questions
- **Issues**: For bugs and feature requests
- **Discord/Matrix**: [Coming Soon] Real-time chat

## 🎉 Recognition

Contributors are recognized in:

- `CONTRIBUTORS.md` file
- Release notes
- Project documentation

Thank you for contributing to ninjaUSB-util! 🚀
