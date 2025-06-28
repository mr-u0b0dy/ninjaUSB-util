# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2025 Dharun A P

cmake_minimum_required(VERSION 3.20)

# Read version from VERSION file
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" VERSION_STRING LIMIT_COUNT 1)
string(STRIP "${VERSION_STRING}" VERSION_STRING)

# Parse version components
string(REGEX MATCH "^([0-9]+)\\.([0-9]+)\\.([0-9]+)(-(.+))?$" VERSION_MATCH "${VERSION_STRING}")
if(NOT VERSION_MATCH)
    message(FATAL_ERROR "Invalid version format in VERSION file: ${VERSION_STRING}")
endif()

set(NINJA_USB_VERSION_MAJOR ${CMAKE_MATCH_1})
set(NINJA_USB_VERSION_MINOR ${CMAKE_MATCH_2})
set(NINJA_USB_VERSION_PATCH ${CMAKE_MATCH_3})
set(NINJA_USB_VERSION_BUILD "${CMAKE_MATCH_5}")
if(NOT NINJA_USB_VERSION_BUILD)
    set(NINJA_USB_VERSION_BUILD "")
endif()

set(NINJA_USB_VERSION_STRING "${VERSION_STRING}")

# Set project with version
project(ninja_util VERSION ${NINJA_USB_VERSION_MAJOR}.${NINJA_USB_VERSION_MINOR}.${NINJA_USB_VERSION_PATCH})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Bluetooth)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUDEV REQUIRED libudev)
pkg_check_modules(LIBEVDEV REQUIRED libevdev)

qt_standard_project_setup(REQUIRES 6.5)

# Generate build information
string(TIMESTAMP BUILD_DATE "%Y-%m-%d")
string(TIMESTAMP BUILD_TIME "%H:%M:%S")

# Configure version header
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/inc/version.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/include/version.hpp"
    @ONLY
)

# Main executable
add_executable(${PROJECT_NAME} 
    src/main.cpp
    src/device_manager.cpp
    src/args.cpp
    src/logger.cpp
)

target_include_directories(
    ${PROJECT_NAME} PRIVATE 
    src/inc 
    ${CMAKE_CURRENT_BINARY_DIR}/include
    ${LIBUDEV_INCLUDE_DIRS}
    ${LIBEVDEV_INCLUDE_DIRS}
)

target_link_libraries(
    ${PROJECT_NAME} PRIVATE 
    ${LIBUDEV_LINK_LIBRARIES} 
    ${LIBEVDEV_LINK_LIBRARIES}
    Qt6::Core 
    Qt6::Bluetooth
)

# Optional: Build tests if requested
option(BUILD_TESTS "Build unit tests" OFF)
if(BUILD_TESTS)
    # Test executables
    add_executable(test_device_manager
        tests/test_device_manager.cpp
        src/device_manager.cpp
        src/logger.cpp
    )
    
    add_executable(test_args
        tests/test_args.cpp
        src/args.cpp
        src/logger.cpp
    )
    
    target_include_directories(
        test_device_manager PRIVATE 
        src/inc 
        ${CMAKE_CURRENT_BINARY_DIR}/include
        ${LIBUDEV_INCLUDE_DIRS}
        ${LIBEVDEV_INCLUDE_DIRS}
    )
    
    target_link_libraries(
        test_device_manager PRIVATE 
        ${LIBUDEV_LINK_LIBRARIES} 
        ${LIBEVDEV_LINK_LIBRARIES}
    )
    
    target_include_directories(
        test_args PRIVATE 
        src/inc
        ${CMAKE_CURRENT_BINARY_DIR}/include
    )
    
    enable_testing()
    add_test(NAME device_manager_tests COMMAND test_device_manager)
    add_test(NAME args_tests COMMAND test_args)
endif()
