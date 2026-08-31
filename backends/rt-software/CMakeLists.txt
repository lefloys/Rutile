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

include(CTest)

if(BUILD_TESTING)
	add_executable(rt-software-resource-tests tests/resource.c)
	target_compile_features(rt-software-resource-tests PRIVATE c_std_11)
	target_include_directories(rt-software-resource-tests PRIVATE "src" "src/resource")
	target_link_libraries(rt-software-resource-tests PRIVATE rt-software)
	if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../../build/Debug/triangle.rtslp")
		target_compile_definitions(rt-software-resource-tests PRIVATE
			RTSW_TRIANGLE_ARTIFACT="${CMAKE_CURRENT_LIST_DIR}/../../build/Debug/triangle.rtslp"
		)
	endif()
	if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../../build/Debug/cube-direct.rtslp")
		target_compile_definitions(rt-software-resource-tests PRIVATE
			RTSW_CUBE_ARTIFACT="${CMAKE_CURRENT_LIST_DIR}/../../build/Debug/cube-direct.rtslp"
		)
	endif()
	add_custom_command(TARGET rt-software-resource-tests POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			$<TARGET_FILE:rt-software>
			$<TARGET_FILE_DIR:rt-software-resource-tests>
	)
	add_test(NAME rt-software.resources COMMAND rt-software-resource-tests)
	if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../../build/Debug/triangle.rtslp")
	add_executable(rt-software-interpreter-tests
		tests/interpreter.c
		src/context.c
		src/error.c
			src/interpreter/artifact.c
			src/interpreter/execute.c
		)
		target_compile_features(rt-software-interpreter-tests PRIVATE c_std_11)
		target_compile_definitions(rt-software-interpreter-tests PRIVATE
			RTSW_TRIANGLE_ARTIFACT="${CMAKE_CURRENT_LIST_DIR}/../../build/Debug/triangle.rtslp"
		)
		if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../../build/Debug/cube-direct.rtslp")
			target_compile_definitions(rt-software-interpreter-tests PRIVATE
				RTSW_CUBE_ARTIFACT="${CMAKE_CURRENT_LIST_DIR}/../../build/Debug/cube-direct.rtslp"
			)
		endif()
		target_include_directories(rt-software-interpreter-tests PRIVATE "src" "src/interpreter")
		add_test(NAME rt-software.interpreter COMMAND rt-software-interpreter-tests)
	endif()
endif()

if(WIN32 AND TARGET rutile AND TARGET rtslc)
	add_subdirectory("example")
endif()
