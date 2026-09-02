add_executable(rutile-07-geometry
    main.cpp
    geometry.rtsl
)
set_source_files_properties(geometry.rtsl PROPERTIES HEADER_FILE_ONLY TRUE)

rutile_configure_example(rutile-07-geometry)
rutile_embed_program(rutile-07-geometry geometry_rtslp "${CMAKE_CURRENT_SOURCE_DIR}/geometry.rtsl")
target_include_directories(rutile-07-geometry PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
