add_executable(rutile-06-rt-debug-software
    main.cpp
    software_debug.rtsl
)
set_source_files_properties(software_debug.rtsl PROPERTIES HEADER_FILE_ONLY TRUE)
rutile_configure_example(rutile-06-rt-debug-software)
rutile_embed_program(rutile-06-rt-debug-software software_debug_rtslp "${CMAKE_CURRENT_SOURCE_DIR}/software_debug.rtsl")
target_include_directories(rutile-06-rt-debug-software PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

if(TARGET rt-debug)
    add_custom_target(rutile-06-rt-debug-software-deploy
        DEPENDS rt-debug
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "$<TARGET_FILE:rt-debug>"
            "$<TARGET_FILE_DIR:rutile-06-rt-debug-software>"
        VERBATIM
    )
    add_dependencies(rutile-06-rt-debug-software rutile-06-rt-debug-software-deploy)
endif()
