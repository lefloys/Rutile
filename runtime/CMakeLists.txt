
add_library("rt-backend-common" STATIC)
target_compile_features("rt-backend-common" PRIVATE c_std_11)
if(WIN32)
	target_sources("rt-backend-common" PRIVATE
	"common/win32/sync.c"
		"common/sync.h"
		"common/atomic.c"
		"common/atomic.h"
	)
else()
	target_sources("rt-backend-common" PRIVATE
		"common/posix/sync.c"
		"common/sync.h"
		"common/atomic.c"
		"common/atomic.h"
	)
endif()



set_target_properties("rt-backend-common" PROPERTIES
    EXCLUDE_FROM_ALL TRUE
    POSITION_INDEPENDENT_CODE ON
)
target_include_directories("rt-backend-common" PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/common"
)
target_link_libraries("rt-backend-common" PRIVATE rutile)
install(TARGETS "rt-backend-common"
    EXPORT RutileTargets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

add_subdirectory("backends" EXCLUDE_FROM_ALL)
add_subdirectory("layers" EXCLUDE_FROM_ALL)
