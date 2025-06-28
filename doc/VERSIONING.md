# Versioning System

## Overview

ninjaUSB-util uses a centralized versioning system based on a single `VERSION` file that propagates version information throughout the entire codebase.

## VERSION File Format

The `VERSION` file contains a single line with semantic versioning format:

```
MAJOR.MINOR.PATCH[-BUILD]
```

Examples:
- `1.0.0` - Release version
- `1.0.0-dev` - Development version  
- `1.2.3-beta` - Beta version
- `2.0.0-rc1` - Release candidate

## How It Works

1. **Version Source**: The `VERSION` file in the project root
2. **CMake Integration**: CMake reads and parses the VERSION file during configuration
3. **Header Generation**: `version.hpp` is auto-generated from `version.hpp.in` template
4. **Build Information**: Build date, time, and compiler info are automatically added

## Version Information Available

The generated `version.hpp` provides:

### Macros
- `NINJA_USB_VERSION_MAJOR` - Major version number
- `NINJA_USB_VERSION_MINOR` - Minor version number  
- `NINJA_USB_VERSION_PATCH` - Patch version number
- `NINJA_USB_VERSION_BUILD` - Build suffix (e.g., "dev", "beta")

### Constants
- `VERSION_STRING` - Complete version string
- `APP_NAME` - Application name
- `DESCRIPTION` - Application description
- `COPYRIGHT` - Copyright information
- `LICENSE` - License information
- `REPOSITORY` - Repository URL
- `BUILD_DATE` - Build date (YYYY-MM-DD)
- `BUILD_TIME` - Build time (HH:MM:SS)
- `BUILD_COMPILER` - Compiler information

### Functions
- `get_version_info()` - Complete formatted version information
- `get_version()` - Just the version number
- `get_build_info()` - Build details
- `get_version_components()` - Version as tuple

## Usage in Code

```cpp
#include "version.hpp"

// Get version string
std::cout << version::VERSION_STRING << std::endl;

// Get complete version info
std::cout << version::get_version_info() << std::endl;

// Get individual components
auto [major, minor, patch, build] = version::get_version_components();
```

## Updating the Version

1. Edit the `VERSION` file with the new version
2. Run CMake configure: `cmake ..`
3. Build: `make`

The version change will automatically propagate to all code that includes `version.hpp`.

## Integration Points

The version system is used in:
- Command-line argument parsing (`--version` flag)
- Application startup logging
- Help text display
- Error messages
- Build information display

## Development Workflow

- Use `-dev` suffix during development: `1.0.0-dev`
- Use `-beta`, `-rc1` etc. for pre-releases
- Use clean version for releases: `1.0.0`

This centralized approach ensures version consistency across the entire codebase and makes releases simple and error-free.
