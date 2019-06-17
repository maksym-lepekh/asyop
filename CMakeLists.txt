cmake_minimum_required(VERSION 3.9)

include(${CMAKE_CURRENT_BINARY_DIR}/conan_paths.cmake OPTIONAL)

add_subdirectory(lib)
add_subdirectory(tests)
