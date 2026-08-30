add_executable(rutile-03-plasma-lab
    main.cpp
    plasma.rtsl
)
set_source_files_properties(plasma.rtsl PROPERTIES HEADER_FILE_ONLY TRUE)

rutile_configure_example(rutile-03-plasma-lab)
rutile_embed_program(rutile-03-plasma-lab plasma_rtslp "${CMAKE_CURRENT_SOURCE_DIR}/plasma.rtsl")
