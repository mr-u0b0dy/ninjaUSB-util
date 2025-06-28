// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2025 Dharun A P

#pragma once

#include <string>

namespace version {

// Version information
constexpr const char* MAJOR = "1";
constexpr const char* MINOR = "0";
constexpr const char* PATCH = "0";
constexpr const char* BUILD = "dev";

constexpr const char* VERSION_STRING = "1.0.0-dev";
constexpr const char* APP_NAME = "ninjaUSB-util";
constexpr const char* DESCRIPTION = "USB keyboard to BLE bridge utility";
constexpr const char* COPYRIGHT = "Copyright (c) 2025 Dharun A P";
constexpr const char* LICENSE = "Licensed under the Apache License 2.0";
constexpr const char* REPOSITORY = "https://github.com/mr-u0b0dy/ninjaUSB-util";

/**
 * @brief Get formatted version string
 * @return Complete version information
 */
inline std::string get_version_info() {
    return std::string(APP_NAME) + " " + VERSION_STRING + "\n" +
           DESCRIPTION + "\n" +
           COPYRIGHT + "\n" +
           LICENSE + "\n" +
           "Repository: " + REPOSITORY;
}

/**
 * @brief Get short version string
 * @return Just the version number
 */
inline std::string get_version() {
    return VERSION_STRING;
}

/**
 * @brief Get build information
 * @return Build details including compiler and date
 */
inline std::string get_build_info() {
    return std::string("Built with ") + 
#ifdef __clang__
           "Clang " + __clang_version__ +
#elif defined(__GNUC__)
           "GCC " + __VERSION__ +
#else
           "Unknown compiler" +
#endif
           " on " + __DATE__ + " " + __TIME__;
}

} // namespace version
