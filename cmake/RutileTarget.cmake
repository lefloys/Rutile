include_guard(GLOBAL)

function(rutile_register_runtime_target target)
    if(NOT TARGET "${target}")
        message(FATAL_ERROR "Unknown Rutile runtime target '${target}'")
    endif()

    if(NOT TARGET "Rutile::${target}")
        add_library("Rutile::${target}" ALIAS "${target}")
    endif()

    install(TARGETS "${target}"
        EXPORT RutileTargets
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    )
endfunction()

function(rutile_configure_shared_library target)
    if(NOT TARGET "${target}")
        message(FATAL_ERROR "Unknown Rutile shared library target '${target}'")
    endif()

    get_target_property(_sources "${target}" SOURCES)
    foreach(_source IN LISTS _sources)
        if(_source MATCHES "\\.c$")
            target_compile_features("${target}" PRIVATE c_std_11)
            break()
        endif()
    endforeach()
    foreach(_source IN LISTS _sources)
        if(_source MATCHES "\\.(cc|cpp|cxx)$")
            target_compile_features("${target}" PRIVATE cxx_std_23)
            break()
        endif()
    endforeach()
    set_target_properties("${target}" PROPERTIES
        CXX_EXTENSIONS OFF
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
    rutile_register_runtime_target("${target}")
endfunction()

function(rutile_use_runtime target)
    if(NOT TARGET "${target}")
        message(FATAL_ERROR "Unknown target '${target}'")
    endif()

    foreach(runtime_target IN LISTS ARGN)
        if(NOT TARGET "${runtime_target}")
            message(FATAL_ERROR
                "Target '${target}' requested unavailable Rutile runtime '${runtime_target}'")
        endif()

        get_target_property(_runtime_deploy_files "${runtime_target}" RUTILE_RUNTIME_DEPLOY_FILES)
        if(_runtime_deploy_files STREQUAL "_runtime_deploy_files-NOTFOUND")
            set(_runtime_deploy_files)
        endif()
        set(_runtime_deploy_commands)
        foreach(_runtime_deploy_file IN LISTS _runtime_deploy_files)
            list(APPEND _runtime_deploy_commands
                COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                    "${_runtime_deploy_file}"
                    "$<TARGET_FILE_DIR:${target}>"
            )
        endforeach()

        # This target has no output on purpose: IDE builds execute it every
        # time the executable is built/launched, keeping copied runtimes in
        # sync even when the executable itself did not relink.
        set(_runtime_copy_target "${target}-${runtime_target}-copy")
        add_custom_target("${_runtime_copy_target}"
            COMMAND "${CMAKE_COMMAND}" -E make_directory
                "$<TARGET_FILE_DIR:${target}>"
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "$<TARGET_FILE:${runtime_target}>"
                "$<TARGET_FILE_DIR:${target}>"
            ${_runtime_deploy_commands}
            DEPENDS "${runtime_target}"
            VERBATIM
        )
        add_dependencies("${target}" "${_runtime_copy_target}")
    endforeach()
endfunction()
