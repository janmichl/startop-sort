cmake_minimum_required(VERSION 2.8.3)

project(startop-sorting)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_BUILD_TYPE       Debug)
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} " -std=c++11 -pthread ")

# top-level target
add_executable(main main.cpp)
