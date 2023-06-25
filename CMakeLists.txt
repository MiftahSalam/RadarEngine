cmake_minimum_required(VERSION 3.10)
project(RadarEngine
    LANGUAGES CXX
    VERSION 3.0)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 COMPONENTS Network OpenGL REQUIRED)

add_subdirectory(arpa)
add_subdirectory(shared)
add_subdirectory(stream)

add_library(RadarEngine STATIC)

target_include_directories(RadarEngine PRIVATE "${PROJECT_SOURCE_DIR}")


target_link_libraries(RadarEngine PRIVATE Qt5::Network Qt5::OpenGL
)
