cmake_minimum_required(VERSION 3.10)
message(STATUS "Build in cc folder")

set(CMAKE_C_FLAGS "-m32 -g -Wall -ansi -std=c90 ${CMAKE_C_FLAGS}")

aux_source_directory(. DIR_CC_SRC)
include_directories(.)

add_executable(cc ${DIR_CC_SRC})