project(alcazar-gen)
cmake_minimum_required(VERSION 2.8)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_CXX_FLAGS "-W -Wall -Werror -std=c++0x -O2")

include_directories(${PROJECT_SOURCE_DIR}/src)
add_executable(alcazar-gen
  src/main.cpp
  src/board.cpp
  src/generator.cpp
  src/path.cpp
  src/wall.cpp
  src/minisat/core/Solver.cc
  src/minisat/utils/System.cc
)
