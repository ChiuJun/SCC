# cmake_minimum_required(VERSION <specify CMake version here>)
cmake_minimum_required(VERSION 3.10)

project(demo C)

set(CMAKE_C_STANDARD 90)

include_directories(include)
set(C_SRC lex.c expr.c error.c decl.c stmt.c main.c)

add_executable(demo ${C_SRC})