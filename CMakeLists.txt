cmake_minimum_required(VERSION 3.10)
project(klingon)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/VulkanMemoryAllocator/build/install/)

find_package(Vulkan REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)

# GLSL shader compilation
function(compile_compute_shader shader_file output_file)
    add_custom_command(
        OUTPUT ${output_file}
        COMMAND glslc -fshader-stage=compute ${CMAKE_CURRENT_SOURCE_DIR}/${shader_file} -o ${output_file}
        DEPENDS ${shader_file}
        COMMENT "Compiling GLSL shader: ${shader_file}"
    )
endfunction()

# List of shaders to compile
set(SHADERS
    shaders/grid.glsl
)

# Make a directory for the compiled shaders, then compile them
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/shaders)

set(COMPILED_SHADERS)
foreach(SHADER ${SHADERS})
    cmake_path(GET SHADER STEM SHADER_NAME)
    set(OUTPUT_SHADER ${CMAKE_CURRENT_BINARY_DIR}/shaders/${SHADER_NAME}.spv)
    compile_compute_shader(${SHADER} ${OUTPUT_SHADER})
    list(APPEND COMPILED_SHADERS ${OUTPUT_SHADER})
endforeach()

# Include stb_image_write and VMA
include_directories(${CMAKE_SOURCE_DIR}/lib)

# Add the executable
add_executable(klingon main.cpp)

# Add shaders as dependencies to the executable
add_custom_target(shaders DEPENDS ${COMPILED_SHADERS})
add_dependencies(klingon shaders)
target_link_libraries(klingon Vulkan::Vulkan GPUOpen::VulkanMemoryAllocator)