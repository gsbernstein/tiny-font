cmake_minimum_required(VERSION 3.20)

project(tinyfont_ibmf_sdl_example LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Locate the tiny-font repo root from this example dir
get_filename_component(TINY_FONT_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)

message(STATUS "tiny-font root: ${TINY_FONT_ROOT}")

add_subdirectory("${TINY_FONT_ROOT}" ${CMAKE_CURRENT_BINARY_DIR}/tiny-font)
target_compile_definitions(tiny-font PUBLIC
    CONFIG_TINYFONT_IBMF=1
    CONFIG_TINYFONT_PIXEL_RESOLUTION_ONE_BIT=1
)

# cpp-misc libraries
include(FetchContent)
FetchContent_Declare(cpp_misc
    GIT_REPOSITORY https://github.com/sindarin-inc/cpp-misc
    GIT_TAG main
)
FetchContent_MakeAvailable(cpp_misc)

add_subdirectory("${cpp_misc_SOURCE_DIR}/idflog" ${CMAKE_CURRENT_BINARY_DIR}/idflog)

# SDL2
find_package(SDL2 REQUIRED)

add_executable(ibmf_sdl
    ${CMAKE_CURRENT_LIST_DIR}/main.cpp
)
target_link_libraries(ibmf_sdl PRIVATE tiny-font SDL2::SDL2)
