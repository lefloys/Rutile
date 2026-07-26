cmake_minimum_required(VERSION 3.28.0)

project(rt-opengl C CXX)

include("${RUTILE_SOURCE_DIR}/cmake/RutileTarget.cmake")

find_package(glfw3 CONFIG REQUIRED)

add_library(${PROJECT_NAME} SHARED)

target_sources(${PROJECT_NAME} PRIVATE
    "src/config.h"
    "src/types.h"
    "src/core.c"
    "src/core.h"
    "src/context.c"
    "src/context.h"
    "src/execution.cpp"
    "src/execution_commands.cpp"
    "src/execution.h"
    "src/execution_internal.hpp"
    "src/error.c"
    "src/error.h"
    "../../common/sync.h"
    "$<$<PLATFORM_ID:Windows>:${CMAKE_CURRENT_SOURCE_DIR}/../../common/win32/sync.c>"
    "$<$<NOT:$<PLATFORM_ID:Windows>>:${CMAKE_CURRENT_SOURCE_DIR}/../../common/posix/sync.c>"
    "src/ext/glfw/swapchain.c"
    "src/ext/glfw/swapchain.h"
    "src/glfw/glfw.c"
    "src/glfw/glfw.h"
    "src/resource/resource.c"
    "src/resource/resource.h"
    "src/resource/buffer.c"
    "src/resource/buffer.h"
    "src/resource/command_buffer.cpp"
    "src/resource/command_buffer.h"
    "src/resource/framebuffer.c"
    "src/resource/framebuffer.h"
    "src/resource/graphics_program.c"
    "src/resource/graphics_program.h"
    "../rt-vulkan/src/rtsl_spirv.cpp"
    "../rt-vulkan/src/rtsl_spirv.h"
    "src/rtsl_glsl.cpp"
    "src/rtsl_glsl.h"
    "${RUTILE_SOURCE_DIR}/RTSL/transpilers/glsl/src/glsl.cpp"
    "${RUTILE_SOURCE_DIR}/RTSL/transpilers/spirv/src/spirv.cpp"
    "src/resource/queue.c"
    "src/resource/queue.h"
    "src/resource/swapchain.c"
    "src/resource/swapchain.h"
    "src/resource/texture.c"
    "src/resource/texture.h"
    
    "src/platform/proc.h"
    "src/platform/context.h"
    "src/platform/wgl/context.c"
    "src/glad/gl.c"
    "src/glad/gl.h"
    "src/KHR/khrplatform.h"
    "src/types.h")

rutile_configure_shared_library(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PRIVATE
    "src"
    "../../common"
    "../../bindings/c/include"
    "../rt-vulkan/src"
    "${RUTILE_SOURCE_DIR}/RTSL/transpilers/glsl/include"
    "${RUTILE_SOURCE_DIR}/RTSL/transpilers/spirv/include"
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        glfw
        opengl32
        gdi32
        user32
        psapi
        rt-backend-common
        rutile
        rtsl-sdk
)

add_subdirectory("workspace")
