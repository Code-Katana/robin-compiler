cmake_minimum_required(VERSION 3.12)
project(wren-lang)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 11)

# Source files
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Include directories
include_directories(include)

# Native build configuration
# Enforce MinGW generator
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    message(STATUS "Using MinGW")
    set(CMAKE_C_COMPILER "gcc")
    set(CMAKE_CXX_COMPILER "g++")
endif()

add_executable(wren-lang ${SOURCES})