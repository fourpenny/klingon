cmake_minimum_required(VERSION 3.10)
project(klingon)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Vulkan REQUIRED)

# Include stb_image_write
include_directories(${CMAKE_SOURCE_DIR}/lib)

# Add the executable
add_executable(klingon main.cpp)

target_link_libraries(klingon Vulkan::Vulkan)
