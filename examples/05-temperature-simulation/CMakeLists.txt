add_executable(rutile-05-temperature-simulation
    main.cpp
    temperature.rtsl
)
set_source_files_properties(temperature.rtsl PROPERTIES HEADER_FILE_ONLY TRUE)

rutile_configure_example(rutile-05-temperature-simulation)
rutile_embed_program(rutile-05-temperature-simulation temperature_rtslp "${CMAKE_CURRENT_SOURCE_DIR}/temperature.rtsl")
