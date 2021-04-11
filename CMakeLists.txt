cmake_minimum_required(VERSION 3.10)
project(SCC C)

set(CMAKE_C_STANDARD 90)
set(CMAKE_C_FLAGS "-m32 -g ${CMAKE_C_FLAGS}")

include_directories(.)

add_subdirectory(cc)

add_executable(scc scc.c scc.h)
