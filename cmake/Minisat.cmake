include(ExternalProject)

ExternalProject_add(Minisat
    GIT_REPOSITORY "https://github.com/niklasso/minisat.git"
    GIT_TAG "37dc6c67e2af26379d88ce349eb9c4c6160e8543"
    UPDATE_COMMAND ""
    PATCH_COMMAND patch -p1 < ${PROJECT_SOURCE_DIR}/external/minisat.patch
    CONFIGURE_COMMAND ${CMAKE_MAKE_PROGRAM} config prefix=${CMAKE_CURRENT_BINARY_DIR}/external
    BUILD_IN_SOURCE 1
)

add_library(MinisatLib STATIC IMPORTED GLOBAL)
set_target_properties(MinisatLib PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/external/lib/libminisat.a)
add_dependencies(MinisatLib Minisat)

set(Minisat_INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR}/external/include)
set(Minisat_LIBRARIES ${CMAKE_CURRENT_BINARY_DIR}/external/lib/libminisat.a)
