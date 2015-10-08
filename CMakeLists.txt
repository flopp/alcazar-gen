project(alcazar-gen)
cmake_minimum_required(VERSION 2.8)

find_package(Boost 1.36.0 COMPONENTS program_options)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_CXX_FLAGS "-W -Wall -Werror -std=c++0x -O2")

include_directories(${PROJECT_SOURCE_DIR}/src)
add_executable(alcazar-gen
  src/board.cpp
  src/commandline.cpp
  src/generator.cpp
  src/main.cpp
  src/path.cpp
  src/wall.cpp
  src/minisat/core/Solver.cc
  src/minisat/simp/SimpSolver.cc
  src/minisat/utils/System.cc
)

if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  target_link_libraries(alcazar-gen ${Boost_LIBRARIES})
endif()
