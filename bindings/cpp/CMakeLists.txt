add_library(rutile-cpp INTERFACE)
add_library(Rutile::cpp ALIAS rutile-cpp)

target_link_libraries(rutile-cpp INTERFACE Rutile::rutile)
target_include_directories(rutile-cpp
    INTERFACE
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

install(DIRECTORY include/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
install(TARGETS rutile-cpp EXPORT RutileTargets)
