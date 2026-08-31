find_package(glfw3 CONFIG REQUIRED)

set(program_artifact "${CMAKE_CURRENT_BINARY_DIR}/software-demo.rtslp")
set(program_source "${CMAKE_CURRENT_BINARY_DIR}/software-demo-program.c")

add_custom_command(
	OUTPUT "${program_artifact}"
	COMMAND "$<TARGET_FILE:rtslc>" "${CMAKE_CURRENT_SOURCE_DIR}/software-demo.rtsl" --module software-demo --emit-program -o "${program_artifact}"
	DEPENDS rtslc "${CMAKE_CURRENT_SOURCE_DIR}/software-demo.rtsl"
	VERBATIM
)

add_custom_command(
	OUTPUT "${program_source}"
	COMMAND "${CMAKE_COMMAND}"
		"-DINPUT=${program_artifact}"
		"-DOUTPUT=${program_source}"
		"-DSYMBOL=rtsw_demo_program"
		-P "${CMAKE_CURRENT_SOURCE_DIR}/embed_program.cmake"
	DEPENDS "${program_artifact}" "${CMAKE_CURRENT_SOURCE_DIR}/embed_program.cmake"
	VERBATIM
)

add_executable(rt-software-demo
	main.c
	"${program_source}"
	software-demo.rtsl
)
set_source_files_properties("${program_source}" PROPERTIES GENERATED TRUE)
set_source_files_properties(software-demo.rtsl PROPERTIES HEADER_FILE_ONLY TRUE)

target_compile_features(rt-software-demo PRIVATE c_std_11)
target_include_directories(rt-software-demo PRIVATE "${RUTILE_SOURCE_DIR}/include")
target_link_libraries(rt-software-demo PRIVATE rutile glfw)
add_dependencies(rt-software-demo rt-software)

set_target_properties(rt-software-demo PROPERTIES
	ENABLE_EXPORTS ON
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/examples"
)

add_custom_command(TARGET rt-software-demo POST_BUILD
	COMMAND "${CMAKE_COMMAND}" -E copy_if_different
		"$<TARGET_FILE:rt-software>"
		"$<TARGET_FILE_DIR:rt-software-demo>"
	VERBATIM
)
