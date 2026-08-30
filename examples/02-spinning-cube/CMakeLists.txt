add_executable(rutile-02-spinning-cube
    main.cpp
    cube.rtsl
)
set_source_files_properties(cube.rtsl PROPERTIES HEADER_FILE_ONLY TRUE)

rutile_configure_example(rutile-02-spinning-cube)
rutile_embed_program(rutile-02-spinning-cube cube_rtslp "${CMAKE_CURRENT_SOURCE_DIR}/cube.rtsl")
