cmake_minimum_required(VERSION 3.12)
project(rbn)

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE SOURCES "src/*.cpp")

include_directories(include)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(STATUS "Using MinGW")
    set(CMAKE_C_COMPILER gcc)
    set(CMAKE_CXX_COMPILER g++)
endif()

find_package(ZLIB REQUIRED)
find_package(LLVM REQUIRED CONFIG)

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

set(LLVM_USE_STATIC_LIBS OFF)

llvm_map_components_to_libnames(LLVM_LIBS
  core
  support
  irreader
  passes
  analysis
  transformutils
  scalaropts
  instcombine
  ipo
  bitwriter
  bitreader
)

message(STATUS "LLVM Libraries: ${LLVM_LIBS}")

add_executable(rbn ${SOURCES})

target_link_libraries(rbn ${LLVM_LIBS} ZLIB::ZLIB)