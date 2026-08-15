cmake_minimum_required(VERSION 3.28)
project(Rutile VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
enable_testing()

set(RUTILE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(RUTILE_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/Rutile")

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/RutileTarget.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/RTSL/cmake/Rtsl.cmake")

add_subdirectory("RTSL" EXCLUDE_FROM_ALL)

add_library(rutile STATIC
    "source/rutile.c"
    "source/rt_ext_swapchain.c"
    "source/rt_ext_glfw.c"
)
add_library(Rutile::rutile ALIAS rutile)

target_include_directories(rutile
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

add_library(rutile-cpp INTERFACE)
add_library(Rutile::cpp ALIAS rutile-cpp)

target_link_libraries(rutile-cpp INTERFACE Rutile::rutile)
target_include_directories(rutile-cpp
    INTERFACE
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

install(DIRECTORY include/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
install(TARGETS rutile rutile-cpp EXPORT RutileTargets)

add_subdirectory("runtime")
add_subdirectory("examples")
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
