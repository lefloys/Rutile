add_executable(rutile-01-triangle
    main.cpp
    triangle.rtsl
)
set_source_files_properties(triangle.rtsl PROPERTIES HEADER_FILE_ONLY TRUE)
rutile_configure_example(rutile-01-triangle)
rutile_embed_program(rutile-01-triangle triangle_rtslp "${CMAKE_CURRENT_SOURCE_DIR}/triangle.rtsl")
target_include_directories(rutile-01-triangle PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
