cmake_minimum_required(VERSION 3.20)

project(tinyfont_ttf_sdl_example LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

get_filename_component(TINY_FONT_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)

add_subdirectory("${TINY_FONT_ROOT}" ${CMAKE_CURRENT_BINARY_DIR}/tiny-font)
target_compile_definitions(tiny-font PUBLIC
    CONFIG_TINYFONT_TTF=1
    CONFIG_TINYFONT_IBMF=0
    CONFIG_TINYFONT_DISPLAY_DPI=150
    CONFIG_TINYFONT_USE_SPIRAM=0
    CONFIG_TINYFONT_PIXEL_RESOLUTION_EIGHT_BIT=1
)
target_link_libraries(tiny-font PUBLIC freetype)

# cpp-misc libraries
include(FetchContent)
FetchContent_Declare(cpp_misc
    GIT_REPOSITORY https://github.com/sindarin-inc/cpp-misc
    GIT_TAG main
)
FetchContent_MakeAvailable(cpp_misc)

add_subdirectory("${cpp_misc_SOURCE_DIR}/idflog" ${CMAKE_CURRENT_BINARY_DIR}/idflog)

find_package(SDL2 REQUIRED)

set(FT_DISABLE_PNG ON CACHE BOOL "Disable PNG support in FreeType" FORCE)
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "Disable HarfBuzz support in FreeType" FORCE)
set(FT_DISABLE_BROTLI ON CACHE BOOL "Disable Brotli support in FreeType" FORCE)
add_subdirectory(${TINY_FONT_ROOT}/freetype/freetype ${CMAKE_CURRENT_BINARY_DIR}/freetype)

add_executable(ttf_sdl
    ${CMAKE_CURRENT_LIST_DIR}/main.cpp
)

target_link_libraries(ttf_sdl PRIVATE tiny-font SDL2::SDL2 freetype)
