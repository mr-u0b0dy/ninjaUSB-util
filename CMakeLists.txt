# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2025 Dharun A P

cmake_minimum_required(VERSION 3.20)
project(ninja_util)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Bluetooth)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUDEV REQUIRED libudev)
pkg_check_modules(LIBEVDEV REQUIRED libevdev)

qt_standard_project_setup(REQUIRES 6.5)

# Main executable
add_executable(${PROJECT_NAME} 
    src/main.cpp
    src/device_manager.cpp
)

target_include_directories(
    ${PROJECT_NAME} PRIVATE 
    src/inc 
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
    # Test executable
    add_executable(test_device_manager
        tests/test_device_manager.cpp
        src/device_manager.cpp
    )
    
    target_include_directories(
        test_device_manager PRIVATE 
        src/inc 
        ${LIBUDEV_INCLUDE_DIRS}
        ${LIBEVDEV_INCLUDE_DIRS}
    )
    
    target_link_libraries(
        test_device_manager PRIVATE 
        ${LIBUDEV_LINK_LIBRARIES} 
        ${LIBEVDEV_LINK_LIBRARIES}
    )
    
    enable_testing()
    add_test(NAME device_manager_tests COMMAND test_device_manager)
endif()
