cmake_minimum_required(VERSION 3.17)
project(NGGC LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

SET(CMAKE_CXX_FLAGS_DEBUG      "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g -Wpedantic -Wall -Wextra")
SET(CMAKE_CXX_FLAGS_RELEASE    "${CMAKE_CXX_FLAGS_RELEASE} -Ofast")


set(CORE_CPPS "")
add_executable(nggc src/NGGC/core.cpp ${CORE_CPPS})
include_directories(src/NGGC/list)