file(GLOB_RECURSE SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/IBMFDriver/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/TTFDriver/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Misc/*.cpp
)

if(DEFINED ESP_PLATFORM AND ESP_PLATFORM)
    set(compile_options
        -includesdkconfig.h
    )

    idf_component_register(SRCS ${SOURCES}
                        INCLUDE_DIRS "src"
                        REQUIRES freetype idflog)

    target_compile_options(${COMPONENT_TARGET} PUBLIC ${compile_options})
else()
    # Allow the target name to be customized for multiple builds
    if(NOT DEFINED TINY_FONT_TARGET_NAME)
        set(TINY_FONT_TARGET_NAME "tiny-font")
    endif()
    
    add_library(${TINY_FONT_TARGET_NAME} STATIC
        ${SOURCES}
    )
    target_include_directories(${TINY_FONT_TARGET_NAME} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    target_link_libraries(${TINY_FONT_TARGET_NAME} PUBLIC idflog)
endif()

project(tiny-font)
