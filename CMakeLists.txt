cmake_minimum_required(VERSION 3.20)
project(ninja_util)

set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_AUTOMOC ON) set(CMAKE_AUTORCC ON) set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Bluetooth)

qt_standard_project_setup(REQUIRES 6.5)

add_executable(${PROJECT_NAME} src/main.cpp)

set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE
                                                 MACOSX_BUNDLE TRUE)
target_link_libraries(${PROJECT_NAME} Qt6::Core Qt6::Bluetooth)
