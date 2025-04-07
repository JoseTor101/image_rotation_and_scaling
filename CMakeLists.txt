# Minimum CMake version
cmake_minimum_required(VERSION 3.10)

# Project name and language
project(ImageRotationScaling LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable warnings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -g -Wno-missing-field-initializers")

# Include directories
include_directories(${CMAKE_SOURCE_DIR})
include_directories(/usr/include/eigen3)

# Source files for the main application
set(MAIN_SOURCES
    main.cpp
    image.cpp
    stb_wrapper.cpp
)

# Source files for the benchmark
set(BENCHMARK_SOURCES
    benchmark.cpp
    image.cpp
    stb_wrapper.cpp
)

# Add executable for the main application
add_executable(ImageRotationScaling ${MAIN_SOURCES})

# Add executable for the benchmark
add_executable(Benchmark ${BENCHMARK_SOURCES})