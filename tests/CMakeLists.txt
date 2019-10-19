cmake_minimum_required(VERSION 3.9)
project(asyop-tests LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../cmake")
include(${CMAKE_CURRENT_BINARY_DIR}/conan_paths.cmake OPTIONAL)

find_package(Catch2 REQUIRED)
find_package(ASIO REQUIRED)

add_executable(asyop-tests
    main.cpp
    util.cpp
    continuation.cpp
    then.cpp
    op.cpp
    ops.cpp
    value_or_error.cpp
    asio.cpp
    executor.cpp
    thread.cpp)
target_link_libraries(asyop-tests PRIVATE Catch2::Catch2 asyop::asio)
