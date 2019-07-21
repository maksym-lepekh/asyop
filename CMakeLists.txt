cmake_minimum_required(VERSION 3.9)

include(${CMAKE_CURRENT_BINARY_DIR}/conan_paths.cmake OPTIONAL)
include(cmake/build-types.cmake)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/")

add_subdirectory(lib)
add_subdirectory(tests)
