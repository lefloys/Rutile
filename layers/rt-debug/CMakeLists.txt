cmake_minimum_required(VERSION 3.28.0)

project(rt-debug C)

include(CTest)

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

add_executable(rt-debug-inspect "tools/inspect.c" "src/trace.c" "src/debugger.c" "src/web.c")
target_include_directories(rt-debug-inspect PRIVATE "src")
target_compile_features(rt-debug-inspect PRIVATE c_std_11)

if(BUILD_TESTING)
	add_executable(rt-debug-trace-tests "tests/trace.c" "src/trace.c" "src/debugger.c" "src/web.c")
	target_include_directories(rt-debug-trace-tests PRIVATE "src")
	target_compile_features(rt-debug-trace-tests PRIVATE c_std_11)
	add_test(NAME rt-debug.trace COMMAND rt-debug-trace-tests)

	add_executable(rt-debug-tests
		"tests/layer.c"
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
	)
	target_include_directories(rt-debug-tests PRIVATE "src")
	target_compile_features(rt-debug-tests PRIVATE c_std_11)
	add_test(NAME rt-debug.layer COMMAND rt-debug-tests)

	if(WIN32)
	add_executable(rt-debug-web-tests "tests/debugger.c" "src/debugger.c" "src/web.c" "src/trace.c" "src/texture_preview.c" "src/resource/rasterizer.c")
		target_include_directories(rt-debug-web-tests PRIVATE "src")
		target_compile_features(rt-debug-web-tests PRIVATE c_std_11)
		target_link_libraries(rt-debug-web-tests PRIVATE ws2_32 shell32)
		add_test(NAME rt-debug.web COMMAND rt-debug-web-tests)
	endif()
endif()

if(WIN32)
	target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32 shell32)
	target_link_libraries(rt-debug-inspect PRIVATE ws2_32 shell32)
	if(TARGET rt-debug-tests)
		target_link_libraries(rt-debug-tests PRIVATE ws2_32 shell32)
	endif()
	if(TARGET rt-debug-trace-tests)
		target_link_libraries(rt-debug-trace-tests PRIVATE ws2_32 shell32)
	endif()
endif()
