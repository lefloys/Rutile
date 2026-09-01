add_executable(rutile-06-tessellation
    main.cpp
    bezier.rtsl
	control_points.rtsl
)
set_source_files_properties(bezier.rtsl control_points.rtsl PROPERTIES HEADER_FILE_ONLY TRUE)

rutile_configure_example(rutile-06-tessellation)
rutile_embed_program(rutile-06-tessellation bezier_rtslp "${CMAKE_CURRENT_SOURCE_DIR}/bezier.rtsl")
rutile_embed_program(rutile-06-tessellation control_points_rtslp "${CMAKE_CURRENT_SOURCE_DIR}/control_points.rtsl")
target_include_directories(rutile-06-tessellation PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
