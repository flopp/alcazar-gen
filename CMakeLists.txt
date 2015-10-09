project(alcazar-gen)
cmake_minimum_required(VERSION 2.8)

find_package(Boost 1.36.0 COMPONENTS program_options)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_CXX_FLAGS "-W -Wall -Werror -std=c++0x -O2")

include (ExternalProject)
ExternalProject_Add (
    Minisat 
    GIT_REPOSITORY "https://github.com/niklasso/minisat.git"
    GIT_TAG "37dc6c67e2af26379d88ce349eb9c4c6160e8543"
    PATCH_COMMAND patch -p1 < ${PROJECT_SOURCE_DIR}/external/minisat.patch
    CONFIGURE_COMMAND ${CMAKE_MAKE_PROGRAM} config prefix=${CMAKE_CURRENT_BINARY_DIR}/external
    BUILD_IN_SOURCE 1
)
set(Minisat_INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR}/external/include)
set(Minisat_LIBRARIES ${CMAKE_CURRENT_BINARY_DIR}/external/lib/libminisat.a)

include_directories(${PROJECT_SOURCE_DIR}/src)
add_executable(alcazar-gen
  src/board.cpp
  src/commandline.cpp
  src/generator.cpp
  src/main.cpp
  src/path.cpp
  src/wall.cpp
)

include_directories(${Boost_INCLUDE_DIRS} ${Minisat_INCLUDE_DIRS})
target_link_libraries(alcazar-gen ${Boost_LIBRARIES} ${Minisat_LIBRARIES})
add_dependencies(alcazar-gen Minisat)
