add_library(rutile INTERFACE)
add_library(Rutile::rutile ALIAS rutile)

target_include_directories(rutile
    INTERFACE
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

install(DIRECTORY include/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
install(TARGETS rutile EXPORT RutileTargets)
