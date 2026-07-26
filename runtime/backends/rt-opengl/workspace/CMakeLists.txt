add_executable(rt-opengl-workspace
    main.cpp
)

target_compile_features(rt-opengl-workspace PRIVATE cxx_std_23)
set_target_properties(rt-opengl-workspace PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:rt-opengl-workspace>"
    ENABLE_EXPORTS ON
)
target_link_libraries(rt-opengl-workspace PRIVATE rutile)
target_link_libraries(rt-opengl-workspace PRIVATE glfw)
rutile_use_runtime(rt-opengl-workspace rt-opengl)
