add_executable(rutile-05-voxel-renderer
    main.cpp
    terrain.rtsl
    water.rtsl
    world.cpp
)
set_source_files_properties(terrain.rtsl water.rtsl PROPERTIES HEADER_FILE_ONLY TRUE)

rutile_configure_example(rutile-05-voxel-renderer)
rutile_embed_program(rutile-05-voxel-renderer terrain_rtslp "${CMAKE_CURRENT_SOURCE_DIR}/terrain.rtsl")
rutile_embed_program(rutile-05-voxel-renderer water_rtslp "${CMAKE_CURRENT_SOURCE_DIR}/water.rtsl")
