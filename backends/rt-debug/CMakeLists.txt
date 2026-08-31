cmake_minimum_required(VERSION 3.28)

# Native, UI-independent trace and debugger-service support.  The Rutile
# backend target is added once it can execute the reference CPU semantics.
add_library(rt-debug-trace STATIC

	src/trace/trace.c
)
add_library(Rutile::debug-trace ALIAS rt-debug-trace)
target_compile_features(rt-debug-trace PUBLIC c_std_11)
target_include_directories(rt-debug-trace PUBLIC src)

if(BUILD_TESTING)
	add_executable(rt-debug-trace-tests tests/trace.c)
	target_link_libraries(rt-debug-trace-tests PRIVATE Rutile::debug-trace)
	add_test(NAME rt-debug.trace COMMAND rt-debug-trace-tests)
endif()
