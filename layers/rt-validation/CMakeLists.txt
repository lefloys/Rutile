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
    "src/sampler.c"
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
    "src/sampler.h"
    "src/swapchain.h"
    "src/texture.h"
    "src/texture_view.h"
    "src/types.h"
)

target_compile_features(${PROJECT_NAME} PRIVATE c_std_11)
target_include_directories(${PROJECT_NAME} PRIVATE "${RUTILE_SOURCE_DIR}/include")
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

if(BUILD_TESTING)
    add_executable(rt-validation-layer-tests "tests/layer.c")
    target_compile_features(rt-validation-layer-tests PRIVATE c_std_11)
    target_include_directories(rt-validation-layer-tests PRIVATE "${RUTILE_SOURCE_DIR}/include")
    target_link_libraries(rt-validation-layer-tests PRIVATE ${PROJECT_NAME})
    set_target_properties(rt-validation-layer-tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    add_test(NAME rt-validation-layer.tests COMMAND rt-validation-layer-tests)
endif()
