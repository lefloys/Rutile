add_executable(rutile-08-voxel-renderer
    main.cpp
    voxel.rtsl
    world.cpp
)
set_source_files_properties(voxel.rtsl PROPERTIES HEADER_FILE_ONLY TRUE)

rutile_configure_example(rutile-08-voxel-renderer)
rutile_embed_program(rutile-08-voxel-renderer voxel_rtslp "${CMAKE_CURRENT_SOURCE_DIR}/voxel.rtsl")
