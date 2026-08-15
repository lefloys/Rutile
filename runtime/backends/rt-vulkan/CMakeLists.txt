cmake_minimum_required(VERSION 3.28.0)

project(rt-vulkan C CXX)

include("${RUTILE_SOURCE_DIR}/cmake/RutileTarget.cmake")
find_package(Vulkan REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(Threads REQUIRED)
find_package(SPIRV-Headers CONFIG REQUIRED)
find_package(volk CONFIG REQUIRED)

add_library(${PROJECT_NAME} SHARED)
target_compile_definitions(${PROJECT_NAME} PRIVATE
    VK_NO_PROTOTYPES
    VMA_STATIC_VULKAN_FUNCTIONS=0
    VMA_DYNAMIC_VULKAN_FUNCTIONS=1
    "$<$<CONFIG:Debug>:RTVK_ENABLE_VULKAN_VALIDATION>"
)

target_sources(${PROJECT_NAME} PRIVATE
    "src/core.c"
    "src/error.c"
    "../../common/sync.h"
    "$<$<PLATFORM_ID:Windows>:${CMAKE_CURRENT_SOURCE_DIR}/../../common/win32/sync.c>"
    "$<$<NOT:$<PLATFORM_ID:Windows>>:${CMAKE_CURRENT_SOURCE_DIR}/../../common/posix/sync.c>"
    
    "src/atomic.cpp"
    
    
    "src/vma.cpp"
    "src/atomic.h"
    "src/resource/buffer.c"
    "src/resource/buffer.h"
    
    
    "src/resource/command_buffer.c"
    "src/resource/command_buffer.h"
    "src/resource/framebuffer.c"
    "src/resource/framebuffer.h"
    "src/resource/graphics_program.c"
    "src/resource/graphics_program.h"
    "src/rtsl_spirv.cpp"
    "src/rtsl_spirv.h"
    "${RUTILE_SOURCE_DIR}/RTSL/transpilers/spirv/src/spirv.cpp"
    "src/resource/queue.c"
    "src/resource/queue.h"
    "src/resource/resource.c"
    "src/resource/resource.h"
    "src/resource/texture.c"
    "src/resource/texture.h"
    "src/resource/swapchain.c"
    "src/resource/swapchain.h"
    "src/glfw/glfw.c"
    "src/glfw/glfw.h"
    "src/glfw/swapchain.c"
    "src/glfw/swapchain.h"
    "src/error.h"
    "src/core.h"
    "src/context.h"
    "src/context.c"
)
target_include_directories(${PROJECT_NAME} PRIVATE
    "src"
    "../../common"
    "../../../include"
    "${RUTILE_SOURCE_DIR}/RTSL/transpilers/spirv/include"
)

rutile_configure_shared_library(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        volk::volk
        GPUOpen::VulkanMemoryAllocator
        rt-backend-common
        rutile
        rtsl-sdk
        $<$<TARGET_EXISTS:Vulkan::Headers>:Vulkan::Headers>
        ${CMAKE_DL_LIBS}
        Threads::Threads
)
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE psapi)
endif()
