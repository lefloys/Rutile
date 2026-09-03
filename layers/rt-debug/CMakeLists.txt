cmake_minimum_required(VERSION 3.28.0)

project(rt-debug C)

add_library(${PROJECT_NAME} SHARED)
target_compile_definitions(${PROJECT_NAME} PRIVATE RT_BUILD_DLL)
target_sources(${PROJECT_NAME} PRIVATE
	"src/entry.c"
	"src/core.c"
	"src/command_buffer.c"
	"src/queue.c"
	"src/framebuffer.c"
	"src/program.c"
	"src/buffer.c"
	"src/texture.c"
	"src/texture_preview.c"
	"src/resource/rasterizer.c"
	"src/sampler.c"
	"src/swapchain.c"
	"src/state.c"
	"src/trace.c"
	"src/debugger.c"
	"src/web.c"
	"src/procs.h"
	"src/next.h"
	"src/debugger.h"
	"src/web.h"
	"src/rutile.h"
	"src/rt_swapchain.h"
	"src/rt_glfw_swapchain.h"
	"src/trace.h"
	"src/texture_preview.h"
	"src/resource/rasterizer.h"
)
target_compile_features(${PROJECT_NAME} PRIVATE c_std_11)
set_target_properties(${PROJECT_NAME} PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

if(WIN32)
	target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32 shell32)
endif()
