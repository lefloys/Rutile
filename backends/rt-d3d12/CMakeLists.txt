cmake_minimum_required(VERSION 3.28)
project(rt-d3d12 LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "rt-d3d12 requires Windows")
endif()

add_library(rt-d3d12 SHARED)

target_sources(rt-d3d12 PRIVATE
    "src/config.hpp"
    "src/context.cpp"
    "src/context.hpp"
    "src/core.cpp"
    "src/core.hpp"
    "src/error.cpp"
    "src/error.hpp"
    "src/glfw.cpp"
    "src/glfw.hpp"
    "src/init.cpp"
    "src/resource/buffer.cpp"
    "src/resource/buffer.hpp"
    "src/resource/command_buffer.cpp"
    "src/resource/command_buffer.hpp"
    "src/resource/framebuffer.cpp"
    "src/resource/framebuffer.hpp"
    "src/resource/program.cpp"
    "src/resource/program.hpp"
    "src/resource/queue.cpp"
    "src/resource/queue.hpp"
    "src/resource/resource.cpp"
    "src/resource/resource.hpp"
    "src/resource/swapchain.cpp"
    "src/resource/swapchain.hpp"
    "src/resource/texture.cpp"
    "src/resource/texture.hpp"
    "src/rutile.hpp"
    "src/sync.cpp"
    "src/sync.hpp"
)

target_compile_features(rt-d3d12 PRIVATE cxx_std_23)
target_compile_definitions(rt-d3d12 PRIVATE
    "$<$<CONFIG:Debug>:RTD3D12_ENABLE_D3D12_VALIDATION>"
)
target_include_directories(rt-d3d12 PRIVATE "src")
target_link_libraries(rt-d3d12 PRIVATE
    d3d12
    dxgi
    dxguid
)

find_library(_rtd3d12_dxcompiler_lib dxcompiler)
if(_rtd3d12_dxcompiler_lib)
    target_link_libraries(rt-d3d12 PRIVATE "${_rtd3d12_dxcompiler_lib}")
else()
    target_link_libraries(rt-d3d12 PRIVATE dxcompiler)
endif()

find_file(_rtd3d12_dxcompiler_dll dxcompiler.dll HINTS
    "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin"
    "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/bin"
    "${CMAKE_WINDOWS_KITS_10_DIR}/Redist/D3D/x64"
    "C:/Program Files (x86)/Windows Kits/10/Redist/D3D/x64"
    "$ENV{VULKAN_SDK}/Bin"
)
find_file(_rtd3d12_dxil_dll dxil.dll HINTS
    "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin"
    "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/bin"
    "${CMAKE_WINDOWS_KITS_10_DIR}/Redist/D3D/x64"
    "C:/Program Files (x86)/Windows Kits/10/Redist/D3D/x64"
    "$ENV{VULKAN_SDK}/Bin"
)
if(NOT _rtd3d12_dxcompiler_dll OR NOT _rtd3d12_dxil_dll)
    message(FATAL_ERROR "rt-d3d12 requires redistributable dxcompiler.dll and dxil.dll")
endif()

set_property(TARGET rt-d3d12 PROPERTY RUTILE_RUNTIME_DEPLOY_FILES
    "${_rtd3d12_dxcompiler_dll}"
    "${_rtd3d12_dxil_dll}"
)
add_custom_command(TARGET rt-d3d12 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${_rtd3d12_dxcompiler_dll}"
        "${_rtd3d12_dxil_dll}"
        "$<TARGET_FILE_DIR:rt-d3d12>"
)

set_target_properties(rt-d3d12 PROPERTIES
    CXX_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
