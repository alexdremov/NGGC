cmake_minimum_required(VERSION 3.17)
project(NGGC LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g -Wpedantic -Wall -Wextra -I/usr/local/include")
SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Ofast -I/usr/local/include")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}  -L/usr/local/lib")

message("${CMAKE_INSTALL_BINDIR}")

include_directories(src src/helpers src/NGGC src/core/bgen src/core/eloquent/ASMStructure /usr/local/include)

set(CORE_CPPS "src/core.cpp")
add_executable(nggc ${CORE_CPPS})

set(CORELOL_CPPS "src/corelol.cpp")
add_executable(nggclol ${CORELOL_CPPS})

target_link_libraries(nggc MachOBuilder )
target_link_libraries(nggclol MachOBuilder)

add_custom_command(TARGET nggc
        PRE_BUILD
        MAIN_DEPENDENCY ${CMAKE_SOURCE_DIR}/src/core/eloquent/ASMStructure/ElCommand_template.h
        DEPENDS ${CMAKE_SOURCE_DIR}/src/core/eloquent/ASMStructure/ElCommand.h
        COMMAND cd ${CMAKE_SOURCE_DIR}/src/core/eloquent/ASMStructure && python3 eloquentGen.py)

add_custom_command(TARGET nggc
        POST_BUILD
        MAIN_DEPENDENCY ${CMAKE_SOURCE_DIR}/src/stdlib/stdlib.cpp
        COMMAND cd ${CMAKE_SOURCE_DIR}/src/stdlib/ && clang -Wl,-undefined -Wl,dynamic_lookup -dynamiclib stdlib.cpp -o nggstdlib.dylib && mv nggstdlib.dylib /usr/local/lib/nggstdlib.dylib)

add_custom_command(TARGET nggc
        POST_BUILD
        MAIN_DEPENDENCY ${CMAKE_SOURCE_DIR}/src/stdlib/stdlib.cpp
        COMMAND cd ${CMAKE_SOURCE_DIR}/src/stdlib/ && clang stdlib.cpp -c -o nggstdlib.o && mv nggstdlib.o /usr/local/lib/nggstdlib.o)


install(TARGETS nggc
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")

install(TARGETS nggclol
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")