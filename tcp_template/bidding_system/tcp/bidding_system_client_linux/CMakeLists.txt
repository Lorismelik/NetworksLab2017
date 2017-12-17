cmake_minimum_required(VERSION 2.8)
project(bidding_system_client_linux)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -pthread")

set(SOURCE_FILES main.c)
add_executable(bidding_system_client_linux ${SOURCE_FILES})

