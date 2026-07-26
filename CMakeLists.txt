cmake_minimum_required(VERSION 3.28)
project(Rutile VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

enable_testing()
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(RUTILE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(RUTILE_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/Rutile")

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/RutileTarget.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/RTSL/cmake/Rtsl.cmake")

add_subdirectory("RTSL" EXCLUDE_FROM_ALL)
add_subdirectory("bindings/c")
add_subdirectory("runtime")
add_subdirectory("examples" EXCLUDE_FROM_ALL)

install(EXPORT RutileTargets
    NAMESPACE Rutile::
    DESTINATION "${RUTILE_INSTALL_CMAKEDIR}"
)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/RutileConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/RutileConfig.cmake"
    INSTALL_DESTINATION "${RUTILE_INSTALL_CMAKEDIR}"
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/RutileConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY SameMajorVersion
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/RutileConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/RutileConfigVersion.cmake"
    DESTINATION "${RUTILE_INSTALL_CMAKEDIR}"
)
