cmake_minimum_required(VERSION 2.8)
project(server_multithread)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")

set(SOURCE_FILES main.c)
add_executable(server_multithread ${SOURCE_FILES})
