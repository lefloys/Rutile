add_executable(rt-opengl-workspace
    main.cpp
)

target_compile_features(rt-opengl-workspace PRIVATE cxx_std_23)
target_include_directories(rt-opengl-workspace PRIVATE "../../../include")
set_target_properties(rt-opengl-workspace PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:rt-opengl-workspace>"
    ENABLE_EXPORTS ON
)
target_link_libraries(rt-opengl-workspace PRIVATE glfw)
add_dependencies(rt-opengl-workspace rt-opengl)
add_custom_command(TARGET rt-opengl-workspace POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "$<TARGET_FILE:rt-opengl>"
        "$<TARGET_FILE_DIR:rt-opengl-workspace>"
)
