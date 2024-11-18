cmake_minimum_required(VERSION 3.10)
project(GrayscaleRenderer)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Include stb_image_write
include_directories(${CMAKE_SOURCE_DIR}/lib)

# Add the executable
add_executable(grayscale_renderer main.cpp)
