cmake_minimum_required(VERSION 3.20)

project(tinyfont_tests LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

get_filename_component(TINY_FONT_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

enable_testing()

add_subdirectory(${TINY_FONT_ROOT}/tests/Catch2 ${CMAKE_CURRENT_BINARY_DIR}/Catch2)

# cpp-misc libraries
include(FetchContent)
FetchContent_Declare(cpp_misc
    GIT_REPOSITORY https://github.com/sindarin-inc/cpp-misc
    GIT_TAG main
)
FetchContent_MakeAvailable(cpp_misc)

add_subdirectory("${cpp_misc_SOURCE_DIR}/idflog" ${CMAKE_CURRENT_BINARY_DIR}/idflog)

find_package(PNG REQUIRED)

# Build tiny-font for IBMF tests
set(TINY_FONT_TARGET_NAME "tiny-font-ibmf" CACHE STRING "Target name for IBMF build" FORCE)
add_subdirectory("${TINY_FONT_ROOT}" ${CMAKE_CURRENT_BINARY_DIR}/tiny-font-ibmf)
target_compile_definitions(tiny-font-ibmf PUBLIC
    CONFIG_TINYFONT_TTF=0
    CONFIG_TINYFONT_IBMF=1
    CONFIG_TINYFONT_PIXEL_RESOLUTION_ONE_BIT=1
    CONFIG_TINYFONT_USE_SPIRAM=0
)
add_executable(tests_ibmf ${CMAKE_CURRENT_LIST_DIR}/TestIBMF.cpp ${CMAKE_CURRENT_LIST_DIR}/ImageIO.cpp)
target_link_libraries(tests_ibmf PRIVATE catch2 PNG::PNG tiny-font-ibmf)
target_compile_definitions(tests_ibmf PRIVATE
    GOLDEN_DIR="${CMAKE_CURRENT_LIST_DIR}/Images"
)
add_test(NAME ibmf_render COMMAND tests_ibmf)

# Build tiny-font for TTF tests
set(FT_DISABLE_PNG ON CACHE BOOL "Disable PNG" FORCE)
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "Disable HarfBuzz" FORCE)
set(FT_DISABLE_BROTLI ON CACHE BOOL "Disable Brotli" FORCE)
add_subdirectory(${TINY_FONT_ROOT}/freetype/freetype ${CMAKE_CURRENT_BINARY_DIR}/freetype)

set(TINY_FONT_TARGET_NAME "tiny-font-ttf" CACHE STRING "Target name for TTF build" FORCE)
add_subdirectory("${TINY_FONT_ROOT}" ${CMAKE_CURRENT_BINARY_DIR}/tiny-font-ttf)
target_compile_definitions(tiny-font-ttf PUBLIC
    CONFIG_TINYFONT_TTF=1
    CONFIG_TINYFONT_IBMF=0
    CONFIG_TINYFONT_PIXEL_RESOLUTION_ONE_BIT=0
    CONFIG_TINYFONT_PIXEL_RESOLUTION_EIGHT_BIT=1
    CONFIG_TINYFONT_DISPLAY_DPI=150
    CONFIG_TINYFONT_USE_SPIRAM=0
)
target_link_libraries(tiny-font-ttf PUBLIC freetype)

add_executable(tests_ttf ${CMAKE_CURRENT_LIST_DIR}/TestTTF.cpp ${CMAKE_CURRENT_LIST_DIR}/ImageIO.cpp)
target_link_libraries(tests_ttf PRIVATE catch2 freetype PNG::PNG tiny-font-ttf)
target_compile_definitions(tests_ttf PRIVATE
    GOLDEN_DIR="${CMAKE_CURRENT_LIST_DIR}/Images"
)
add_test(NAME ttf_render COMMAND tests_ttf)

add_executable(tests_utf8 ${CMAKE_CURRENT_LIST_DIR}/TestUTF8Iterator.cpp)
target_link_libraries(tests_utf8 PRIVATE catch2 tiny-font-ibmf)
add_test(NAME utf8_iterator COMMAND tests_utf8)
