cmake_minimum_required(VERSION 3.17)
project(NGGC LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

SET(CMAKE_CXX_FLAGS_DEBUG      "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g -Wpedantic -Wall -Wextra -I/usr/local/include")
SET(CMAKE_CXX_FLAGS_RELEASE    "${CMAKE_CXX_FLAGS_RELEASE} -Ofast")
SET(CMAKE_EXE_LINKER_FLAGS     "${CMAKE_EXE_LINKER_FLAGS} -L/usr/local/lib")

set(CORE_CPPS "src/NGGC/core.cpp")
add_executable(nggc ${CORE_CPPS})
target_link_libraries(nggc MachOBuilder)