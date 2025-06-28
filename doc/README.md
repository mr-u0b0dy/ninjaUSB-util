# Documentation Overview

This document provides an overview of the ninjaUSB-util documentation structure.

## Documentation Structure

### Root Level

- **[README.md](../README.md)** - Quick start guide for users
- **[CONTRIBUTING.md](../CONTRIBUTING.md)** - Quick start guide for contributors
- **[VERSION](../VERSION)** - Single source of truth for version numbers

### doc/ Directory

- **[USER_GUIDE.md](USER_GUIDE.md)** - Complete user guide with installation,
  usage, and troubleshooting
- **[DEVELOPMENT.md](DEVELOPMENT.md)** - Comprehensive development guide
- **[ARCHITECTURE.md](ARCHITECTURE.md)** - System architecture and design
  documentation
- **[TESTING.md](TESTING.md)** - Testing procedures and requirements
- **[VERSIONING.md](VERSIONING.md)** - Version management system details

## For Users

If you want to **use** ninjaUSB-util:

1. Start with **README.md** for quick start
2. Read **USER_GUIDE.md** for comprehensive usage instructions
3. Check command-line options: `./ninja_util --help`

## For Contributors

If you want to **contribute** to ninjaUSB-util:

1. Start with **CONTRIBUTING.md** for quick setup
2. Read **DEVELOPMENT.md** for detailed development guidelines
3. Follow **TESTING.md** for testing procedures
4. Understand **VERSIONING.md** for version management

## For Maintainers

If you **maintain** ninjaUSB-util:

- All guides in `doc/` contain comprehensive information
- Version management is centralized through the VERSION file
- Documentation follows a clear hierarchy from quick-start to detailed guides

## Documentation Philosophy

- **Minimal root files**: Keep the project root clean with essential files only
- **Detailed guides in doc/**: Comprehensive information organized by topic
- **Progressive disclosure**: Quick start → Detailed guides → Specialized topics
- **Single source of truth**: Avoid duplication between documents
