find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(CLI11 CONFIG REQUIRED)
find_package(Stb REQUIRED)

function(rutile_embed_program target symbol source)
    file(READ "${source}" _program_hex HEX)
    string(REGEX REPLACE "([0-9a-fA-F][0-9a-fA-F])" "0x\\1, " _program_bytes "${_program_hex}")

    set(_program_source "${CMAKE_CURRENT_BINARY_DIR}/${symbol}.cpp")
    file(WRITE "${_program_source}"
        "#include \"embedded_program.hpp\"\n\n"
        "static const u08 ${symbol}_data[] = { ${_program_bytes} };\n"
        "extern \"C\" const rt_example_program ${symbol} = { ${symbol}_data, sizeof(${symbol}_data) };\n"
    )
    target_sources("${target}" PRIVATE "${_program_source}")
endfunction()

function(rutile_configure_example target)
    target_compile_features("${target}" PRIVATE cxx_std_23)
    set_target_properties("${target}" PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${target}>"
        ENABLE_EXPORTS ON
    )

    target_include_directories("${target}" PRIVATE
        ${Stb_INCLUDE_DIR}
        "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/shared"
    )

    target_link_libraries("${target}" PRIVATE
        glfw
        glm::glm
        CLI11::CLI11
    )

    target_link_libraries("${target}" PRIVATE rutile)
    foreach(_runtime IN ITEMS
        rt-vulkan
        rt-d3d12
        rt-opengl
        rt-validation-layer
        rt-logging-layer
    )
        if(TARGET "${_runtime}")
            add_dependencies("${target}" "${_runtime}")
            add_custom_command(TARGET "${target}" POST_BUILD
                COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                    "$<TARGET_FILE:${_runtime}>"
                    "$<TARGET_FILE_DIR:${target}>"
                VERBATIM
            )

            get_target_property(_runtime_files "${_runtime}" RUTILE_RUNTIME_DEPLOY_FILES)
            if(_runtime_files)
                add_custom_command(TARGET "${target}" POST_BUILD
                    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                        ${_runtime_files}
                        "$<TARGET_FILE_DIR:${target}>"
                    VERBATIM
                )
            endif()
        endif()
    endforeach()
endfunction()

add_subdirectory("01-triangle")
add_subdirectory("02-spinning-cube")
add_subdirectory("03-plasma-lab")
add_subdirectory("04-galaxy")
add_subdirectory("05-voxel-renderer")

set_property(GLOBAL APPEND PROPERTY RUTILE_EXAMPLE_TARGETS
    "rutile-01-triangle"
    "rutile-02-spinning-cube"
    "rutile-03-plasma-lab"
    "rutile-04-galaxy"
    "rutile-05-voxel-renderer"
)
