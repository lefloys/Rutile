cmake_minimum_required(VERSION 3.28.0)

project(rt-validation-layer C)

add_library(${PROJECT_NAME} SHARED)
target_compile_definitions(${PROJECT_NAME} PRIVATE RT_BUILD_DLL)

target_sources(${PROJECT_NAME} PRIVATE
    "src/buffer.c"
    "src/command_buffer.c"
    "src/core.c"
    "src/entry.c"
    "src/framebuffer.c"
    "src/program.c"
    "src/logger.c"
    "src/queue.c"
    "src/state.c"
    "src/swapchain.c"
    "src/texture.c"
    "src/buffer.h"
    "src/command_buffer.h"
    "src/framebuffer.h"
    "src/program.h"
    "src/handles.h"
    "src/logger.h"
    "src/procs.h"
    "src/queue.h"
    "src/swapchain.h"
    "src/texture.h"
    "src/texture_view.h"
    "src/types.h"
)

target_compile_features(${PROJECT_NAME} PRIVATE c_std_11)
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
