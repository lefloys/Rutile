find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(CLI11 CONFIG REQUIRED)
find_package(Stb REQUIRED)

set(RUTILE_EXAMPLE_RUNTIMES "rt-vulkan" CACHE STRING "Runtime backends built and deployed with examples")

function(rutile_embed_program target symbol source)
    get_filename_component(module_name "${source}" NAME_WE)
    set(program_artifact "${CMAKE_CURRENT_BINARY_DIR}/${symbol}.rtslp")
    set(program_source "${CMAKE_CURRENT_BINARY_DIR}/${symbol}.cpp")

    add_custom_command(
        OUTPUT "${program_artifact}"
        COMMAND "$<TARGET_FILE:rtslc>" "${source}" --module "${module_name}" --emit-program -o "${program_artifact}"
        DEPENDS rtslc "${source}"
        VERBATIM
    )
    add_custom_command(
        OUTPUT "${program_source}"
        COMMAND "${CMAKE_COMMAND}"
            "-DINPUT=${program_artifact}"
            "-DOUTPUT=${program_source}"
            "-DSYMBOL=${symbol}"
            -P "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../cmake/embed_program.cmake"
        DEPENDS "${program_artifact}" "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../cmake/embed_program.cmake"
        VERBATIM
    )
    set_source_files_properties("${program_source}" PROPERTIES GENERATED TRUE)
    target_sources("${target}" PRIVATE "${program_source}")
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
    foreach(_runtime IN LISTS RUTILE_EXAMPLE_RUNTIMES)
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
