cmake_minimum_required(VERSION 3.10)
project(SCC C)

set(CMAKE_C_STANDARD 90)

include_directories(.)

add_executable(scc
        scc.c
        scc.h)
