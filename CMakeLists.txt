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

# Set PROJECT_VERSION for Doxygen to include full version string (with suffixes like "-dev")
set(PROJECT_VERSION "${NINJA_USB_VERSION_STRING}")

# Set PROJECT_NAME for Doxygen based on the CMake project name
set(DOXYGEN_PROJECT_NAME "NinjaUSB Util ")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)

# Check for required system libraries with helpful error messages
pkg_check_modules(LIBUDEV libudev)
if(NOT LIBUDEV_FOUND)
    message(FATAL_ERROR 
        "libudev development library not found!\n"
        "Please install it using:\n"
        "  Ubuntu/Debian: sudo apt-get install libudev-dev\n"
        "  Fedora/RHEL: sudo dnf install systemd-devel\n"
        "  Arch: sudo pacman -S systemd")
endif()

pkg_check_modules(LIBEVDEV libevdev)
if(NOT LIBEVDEV_FOUND)
    message(FATAL_ERROR 
        "libevdev development library not found!\n"
        "Please install it using:\n"
        "  Ubuntu/Debian: sudo apt-get install libevdev-dev\n"
        "  Fedora/RHEL: sudo dnf install libevdev-devel\n"
        "  Arch: sudo pacman -S libevdev")
endif()

# Try Qt6 first, fall back to Qt5 if needed
find_package(Qt6 QUIET COMPONENTS Core Bluetooth)
if(Qt6_FOUND)
    message(STATUS "Using Qt6")
    set(QT_VERSION_MAJOR 6)
    # Use conditional Qt setup based on version availability
    if(Qt6_VERSION VERSION_GREATER_EQUAL "6.5")
        qt_standard_project_setup(REQUIRES 6.5)
    else()
        qt_standard_project_setup()
    endif()
    set(QT_LIBRARIES Qt6::Core Qt6::Bluetooth)
else()
    message(STATUS "Qt6 not found, trying Qt5")
    find_package(Qt5 REQUIRED COMPONENTS Core Bluetooth)
    set(QT_VERSION_MAJOR 5)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
    set(CMAKE_AUTOUIC ON)
    set(QT_LIBRARIES Qt5::Core Qt5::Bluetooth)
endif()

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
    ${QT_LIBRARIES}
)

# Optional: Build tests if requested
option(BUILD_TESTS "Build unit tests" OFF)
option(BUILD_DOCS "Build API documentation with Doxygen" OFF)

if(BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        # Download doxygen-awesome-css theme using FetchContent
        include(FetchContent)
        FetchContent_Declare(doxygen-awesome-css
            GIT_REPOSITORY https://github.com/jothepro/doxygen-awesome-css.git
            GIT_TAG v2.3.3  # Use a specific version tag for reproducible builds
        )
        FetchContent_MakeAvailable(doxygen-awesome-css)
        
        # Set up Doxygen configuration
        set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/.doxygen)
        set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/.doxygen)
        
        # Set theme-related variables for Doxygen configuration
        set(DOXYGEN_AWESOME_CSS ${doxygen-awesome-css_SOURCE_DIR}/doxygen-awesome.css)
        set(DOXYGEN_AWESOME_SIDEBAR_ONLY_CSS ${doxygen-awesome-css_SOURCE_DIR}/doxygen-awesome-sidebar-only.css)
        
        # Copy .doxygen to build directory  
        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
        
        # Create output directory
        file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/doc/api)
        
        # Add documentation target
        add_custom_target(docs
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM
        )
        
        message(STATUS "Doxygen found - documentation can be built with 'make docs'")
        message(STATUS "Using doxygen-awesome-css theme from: ${doxygen-awesome-css_SOURCE_DIR}")
    else()
        message(WARNING "Doxygen not found - documentation target not available")
    endif()
endif()

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
