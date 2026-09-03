cmake_minimum_required(VERSION 3.28)
project(rt-software LANGUAGES C)

add_library(rt-software SHARED
	src/config.h
	src/context.c
	src/context.h
	src/core.c
	src/core.h
	src/error.c
	src/error.h
	src/execution/execution.c
	src/execution/execution.h
	src/interpreter/artifact.c
	src/interpreter/artifact.h
	src/interpreter/execute.c
	src/interpreter/execute.h
	src/resource/buffer.c
	src/resource/buffer.h
	src/resource/command_buffer.c
	src/resource/command_buffer.h
	src/resource/framebuffer.c
	src/resource/framebuffer.h
	src/resource/resource.c
	src/resource/resource.h
	src/resource/sampler.c
	src/resource/sampler.h
	src/resource/queue.c
	src/resource/queue.h
	src/resource/program.c
	src/resource/program.h
	src/resource/texture.c
	src/resource/texture.h
	src/rutile.h
)

if(WIN32)
	target_sources(rt-software PRIVATE
		src/presentation/windows/glfw.c
		src/presentation/windows/glfw.h
		src/presentation/windows/swapchain.c
		src/presentation/windows/swapchain.h
	)
endif()

target_compile_features(rt-software PRIVATE c_std_11)
target_include_directories(rt-software PRIVATE
	"src"
	"src/execution"
	"src/interpreter"
	"src/resource"
)

set_target_properties(rt-software PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
