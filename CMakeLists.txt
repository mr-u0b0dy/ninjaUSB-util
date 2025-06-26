cmake_minimum_required(VERSION 3.20)
project(ninja_util)

set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_AUTOMOC ON) set(CMAKE_AUTORCC ON) set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Bluetooth)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUDEV REQUIRED libudev)
pkg_check_modules(LIBEVDEV REQUIRED libevdev)

qt_standard_project_setup(REQUIRES 6.5)

add_executable(${PROJECT_NAME} src/main.cpp)

# set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE
# MACOSX_BUNDLE TRUE)

target_include_directories(
  ${PROJECT_NAME} PRIVATE src/inc ${LIBUDEV_INCLUDE_DIRS}
                          ${LIBEVDEV_INCLUDE_DIRS})
target_link_libraries(
  ${PROJECT_NAME} PRIVATE ${LIBUDEV_LINK_LIBRARIES} ${LIBEVDEV_LINK_LIBRARIES}
                          Qt6::Core Qt6::Bluetooth)
