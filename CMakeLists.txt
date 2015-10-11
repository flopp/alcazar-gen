project(alcazar-gen)
cmake_minimum_required(VERSION 2.8)

find_package(Boost 1.36.0 COMPONENTS program_options)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
#set(CMAKE_CXX_FLAGS "-W -Wall -Werror -std=c++0x -O2")
set(CMAKE_CXX_FLAGS "-W -Wall -Werror -std=c++11 -O2")

include_directories(${PROJECT_SOURCE_DIR}/src)
add_executable(alcazar-gen
  src/board.cpp
  src/commandline.cpp
  src/generator.cpp
  src/main.cpp
  src/path.cpp
  src/templateBoard.cpp
  src/wall.cpp
)

include(Minisat)
include_directories(${Boost_INCLUDE_DIRS} ${Minisat_INCLUDE_DIRS})
target_link_libraries(alcazar-gen ${Boost_LIBRARIES} ${Minisat_LIBRARIES})
add_dependencies(alcazar-gen MinisatLib)
