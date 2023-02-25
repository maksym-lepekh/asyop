cmake_minimum_required(VERSION 3.24)
project(asyop LANGUAGES CXX VERSION 2.0)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(build-types)

find_program(CCACHE_BIN ccache)
if (CCACHE_BIN)
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_BIN})
endif ()

add_subdirectory(lib)

if(PROJECT_IS_TOP_LEVEL)
    include(CTest)
    if (BUILD_TESTING)
        add_subdirectory(tests)
    endif ()
endif()
