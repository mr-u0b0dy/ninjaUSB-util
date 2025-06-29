# 🚀 CI/CD Pipeline Documentation

This document describes the comprehensive CI/CD pipeline setup for ninjaUSB-util.

## 📋 Pipeline Overview

The CI/CD pipeline consists of multiple GitHub Actions workflows that provide:

- ✅ **Continuous Integration**: Automated building and testing
- 🔍 **Quality Assurance**: Code analysis, formatting, and security scanning  
- 📚 **Documentation**: Automated documentation generation and deployment
- 🚀 **Deployment**: Automated releases and artifact publishing
- 🔧 **Maintenance**: Dependency updates and performance monitoring

## 🔧 Workflow Files

### 1. Main CI/CD Pipeline (`.github/workflows/ci.yml`)

**Triggers:** Push to `main`/`develop`, Pull requests, Tags starting with `v*`

**Jobs:**
- **build-and-test**: Multi-platform builds (Ubuntu 20.04, 22.04, 24.04) with Debug/Release configurations
- **code-quality**: Static analysis with clang-tidy and cppcheck  
- **security-scan**: CodeQL security analysis
- **create-release**: Automated releases from version tags
- **deploy-docs**: GitHub Pages documentation deployment

**Features:**
- Matrix builds across Ubuntu versions and build types
- Comprehensive test execution with `ctest`
- Documentation generation with Doxygen
- Binary artifact uploads for each platform
- Automated release creation from Git tags

### 2. Security & Dependencies (`.github/workflows/security.yml`)

**Triggers:** Weekly schedule (Monday 6 AM), Push to `main`, Pull requests

**Jobs:**
- **dependency-scan**: Trivy vulnerability scanning
- **license-check**: License header compliance verification
- **supply-chain-security**: Dependency security analysis
- **secrets-scan**: TruffleHog secret detection

### 3. Performance & Analysis (`.github/workflows/performance.yml`)

**Triggers:** Push to `main`, Pull requests, Weekly schedule (Monday 2 AM)

**Jobs:**
- **performance-test**: Memory leak detection with Valgrind, CPU profiling, binary size analysis
- **static-analysis**: Include What You Use (IWYU), dead code detection, coverage analysis

### 4. Maintenance (`.github/workflows/dependencies.yml`)

**Triggers:** Weekly schedule (Monday 8 AM), Manual trigger

**Jobs:**
- **update-dependencies**: Qt6/CMake version checks, GitHub Actions updates
- **check-documentation**: Documentation freshness validation, broken link detection
- **lint-workflows**: GitHub Actions workflow linting with actionlint

## 🛠️ Development Tools

### Makefile
A comprehensive Makefile provides convenient commands:

```bash
make help           # Show all available commands
make build          # Build the project
make test           # Run all tests
make dev-setup      # Install development dependencies
make lint           # Run static analysis
make memory-check   # Run Valgrind memory checking
make docs           # Build documentation
make ci-test        # Run full CI test suite locally
```

### Code Quality Tools

- **clang-format**: Consistent code formatting (`.clang-format`)
- **clang-tidy**: Static analysis and modernization suggestions
- **cppcheck**: Additional static analysis
- **Valgrind**: Memory leak detection and profiling
- **Doxygen**: API documentation generation

### Development Environment

- **VS Code DevContainer**: Pre-configured development environment with all tools
- **GitHub Codespaces**: Cloud-based development environment
- **Docker support**: Containerized builds and testing

## 📊 Quality Gates

Every pull request must pass:

1. ✅ **Build**: Successful compilation on all target platforms
2. ✅ **Tests**: All unit tests pass with no failures
3. ✅ **Static Analysis**: No critical issues from clang-tidy/cppcheck
4. ✅ **Security**: No security vulnerabilities detected
5. ✅ **Memory**: No memory leaks detected by Valgrind
6. ✅ **Documentation**: All public APIs documented

## 🚀 Release Process

### Automated Releases

1. **Version Update**: Update `VERSION` file with new version
2. **Tag Creation**: Create and push Git tag (e.g., `git tag v1.2.3`)
3. **Automatic Build**: Pipeline builds binaries for all platforms
4. **Release Creation**: GitHub release created with:
   - Binary artifacts for Ubuntu 20.04, 22.04, 24.04
   - Generated documentation
   - Automated release notes
   - Distribution packages

### Manual Release Steps

```bash
# 1. Update version
echo "1.2.3" > VERSION

# 2. Commit and tag
git add VERSION
git commit -m "Release v1.2.3"
git tag v1.2.3

# 3. Push (triggers release workflow)
git push origin main
git push origin v1.2.3
```

## 🔍 Monitoring & Alerts

### Performance Monitoring
- Weekly performance benchmarks
- Binary size tracking
- Memory usage analysis
- Startup time measurements

### Security Monitoring  
- Weekly vulnerability scans
- Dependency security updates
- Secret detection in commits
- License compliance checking

### Quality Monitoring
- Code coverage tracking
- Static analysis trend monitoring
- Documentation coverage
- Test result trending

## 🐛 Troubleshooting

### Common Issues

**Build Failures:**
- Check Qt6 dependencies are installed
- Verify CMake version (>= 3.20)
- Ensure all system packages are available

**Test Failures:**
- Run tests locally: `make test-verbose`
- Check for memory leaks: `make memory-check`
- Review test logs in CI artifacts

**Security Scan Failures:**
- Review Trivy vulnerability report
- Update dependencies if needed
- Add suppressions for false positives

**Documentation Build Issues:**
- Ensure Doxygen is installed
- Check for syntax errors in comments
- Verify external dependencies are available

### Local Testing

Run the full CI suite locally:
```bash
make ci-test
```

This executes the same checks as the CI pipeline.

## 📈 Metrics & Analytics

### Build Metrics
- Build time trends across platforms
- Success/failure rates
- Resource usage patterns

### Code Quality Metrics
- Lines of code trends
- Test coverage percentage
- Issue density (bugs per KLOC)
- Technical debt indicators

### Security Metrics
- Vulnerability discovery rate
- Time to remediation
- Dependency freshness
- License compliance rate

## 🔧 Configuration Files

| File | Purpose |
|------|---------|
| `.github/workflows/ci.yml` | Main CI/CD pipeline |
| `.github/workflows/security.yml` | Security scanning |
| `.github/workflows/performance.yml` | Performance testing |
| `.github/workflows/dependencies.yml` | Maintenance automation |
| `.github/CONTRIBUTING.md` | Contribution guidelines |
| `.github/pull_request_template.md` | PR template |
| `.github/ISSUE_TEMPLATE/` | Issue templates |
| `.devcontainer/devcontainer.json` | Dev container configuration |
| `.clang-format` | Code formatting rules |
| `Makefile` | Build automation |

## 🎯 Future Enhancements

### Planned Improvements
- [ ] ARM64 build support
- [ ] Windows build support (cross-compilation)
- [ ] Automated dependency updates (Dependabot)
- [ ] Performance regression detection
- [ ] Automated security patching
- [ ] Integration with package managers (APT, Snap)
- [ ] Container image publishing
- [ ] Multi-architecture Docker builds

### Metrics Dashboard
Consider implementing:
- Build success rate dashboard
- Code quality trend visualization  
- Security posture monitoring
- Performance benchmark tracking

## 📞 Support

For pipeline issues:
1. Check [GitHub Actions logs](https://github.com/your-username/ninjaUSB-util/actions)
2. Review [troubleshooting guide](#troubleshooting)
3. Open an issue with `ci` label
4. Contact maintainers via GitHub Discussions

---

**Pipeline Status:** ✅ Active and Monitoring
**Last Updated:** $(date)
**Next Review:** Quarterly
