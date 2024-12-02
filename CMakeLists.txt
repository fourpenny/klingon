cmake_minimum_required(VERSION 3.10)
project(klingon)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/VulkanMemoryAllocator/build/install/)

find_package(Vulkan REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)

# Include stb_image_write and VMA
include_directories(${CMAKE_SOURCE_DIR}/lib)

# Add the executable
add_executable(klingon main.cpp)

target_link_libraries(klingon Vulkan::Vulkan GPUOpen::VulkanMemoryAllocator)