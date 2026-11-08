cmake_minimum_required(VERSION 3.20)
project(italic_lab LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
get_filename_component(TINY_FONT_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)

if(EXISTS "${TINY_FONT_ROOT}/../cpp-misc/idflog/CMakeLists.txt")
    set(cpp_misc_SOURCE_DIR "${TINY_FONT_ROOT}/../cpp-misc")
else()
    include(FetchContent)
    FetchContent_Declare(cpp_misc
        GIT_REPOSITORY https://github.com/sindarin-inc/cpp-misc
        GIT_TAG main
    )
    FetchContent_MakeAvailable(cpp_misc)
endif()
add_subdirectory("${cpp_misc_SOURCE_DIR}/idflog" "${CMAKE_CURRENT_BINARY_DIR}/idflog")
set(TINY_FONT_TARGET_NAME "tiny-font-italic-lab")
add_subdirectory("${TINY_FONT_ROOT}" "${CMAKE_CURRENT_BINARY_DIR}/tiny-font")
target_compile_definitions(tiny-font-italic-lab PUBLIC
    CONFIG_TINYFONT_TTF=0
    CONFIG_TINYFONT_IBMF=1
    CONFIG_TINYFONT_PIXEL_RESOLUTION_ONE_BIT=1
    CONFIG_TINYFONT_USE_SPIRAM=0
)
add_executable(italic-lab main.cpp)
target_link_libraries(italic-lab PRIVATE tiny-font-italic-lab)
target_compile_definitions(italic-lab PRIVATE
    DEFAULT_GLYPH_JSON="${TINY_FONT_ROOT}/src/IBMFFonts/SolSans_75_MediumItalic.json"
)
