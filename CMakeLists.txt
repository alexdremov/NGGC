cmake_minimum_required(VERSION 3.17)
project(NGGC LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g -Wpedantic -Wall -Wextra -I/usr/local/include")
SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Ofast")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}  -L/usr/local/lib")

include_directories(src/Helpers/CppSwiftyList)
include_directories(src/Helpers)
include_directories(src/NGGC)
include_directories(src/core/bgen)
include_directories(src/core/eloquent/ASMStructure)

set(CORE_CPPS "src/core.cpp")
add_executable(nggc ${CORE_CPPS})

target_link_libraries(nggc MachOBuilder)

add_custom_command(TARGET nggc
        PRE_BUILD
        MAIN_DEPENDENCY ${CMAKE_SOURCE_DIR}/src/NGGC/core/eloquent/ASMStructure/ElCommand_template.h
        DEPENDS ${CMAKE_SOURCE_DIR}/src/NGGC/core/eloquent/ASMStructure/ElCommand.h
        COMMAND cd ${CMAKE_SOURCE_DIR}/src/NGGC/core/eloquent/ASMStructure && python3 eloquentGen.py)

add_subdirectory(MachOBuilder)