cmake_minimum_required(VERSION 3.28.0)

project(rt-logging-layer C)

include("${RUTILE_SOURCE_DIR}/cmake/RutileTarget.cmake")

add_library(${PROJECT_NAME} SHARED)
target_compile_definitions(${PROJECT_NAME} PRIVATE RT_BUILD_DLL)

target_sources(${PROJECT_NAME} PRIVATE
    "src/buffer.c"
    "src/command_buffer.c"
    "src/core.c"
    "src/entry.c"
    "src/framebuffer.c"
    "src/graphics_program.c"
    "src/logger.c"
    "src/procs.h"
    "src/queue.c"
    "src/state.c"
    "src/swapchain.c"
    "src/texture.c"
)

rutile_configure_shared_library(${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME} PRIVATE rutile)
