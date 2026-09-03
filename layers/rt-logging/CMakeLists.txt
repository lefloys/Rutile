cmake_minimum_required(VERSION 3.28.0)

project(rt-logging-layer C)

add_library(${PROJECT_NAME} SHARED)
target_compile_definitions(${PROJECT_NAME} PRIVATE RT_BUILD_DLL)

target_sources(${PROJECT_NAME} PRIVATE
    "src/entry.c"
    "src/forward.c"
    "src/logger.c"
    "src/procs.h"
    "src/state.c"
)

target_compile_features(${PROJECT_NAME} PRIVATE c_std_11)
target_include_directories(${PROJECT_NAME} PRIVATE "${RUTILE_SOURCE_DIR}/include")
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
