find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(CLI11 CONFIG REQUIRED)
find_package(Stb REQUIRED)
include("${RUTILE_SOURCE_DIR}/cmake/RutileTarget.cmake")

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
    set(_example_runtimes)
    foreach(_runtime IN ITEMS
        rt-vulkan
        rt-directx12
        rt-opengl
        rt-validation-layer
        rt-logging-layer
    )
        if(TARGET "${_runtime}")
            list(APPEND _example_runtimes "${_runtime}")
        endif()
    endforeach()
    if(_example_runtimes)
        rutile_use_runtime("${target}" ${_example_runtimes})
    endif()
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
