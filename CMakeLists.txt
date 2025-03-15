cmake_minimum_required(VERSION 3.12)
project(rbn)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 14)

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

# Find LLVM
find_package(LLVM REQUIRED CONFIG)

# Include LLVM headers
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Link LLVM libraries
llvm_map_components_to_libnames(LLVM_LIBS core support)
message(STATUS "LLVM Libraries: ${LLVM_LIBS}")

# Create the executable
add_executable(rbn ${SOURCES})

# Link against LLVM
target_link_libraries(rbn ${LLVM_LIBS})
