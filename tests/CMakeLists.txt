cmake_minimum_required(VERSION 3.24)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../cmake")
include(${CMAKE_CURRENT_BINARY_DIR}/conan_paths.cmake OPTIONAL)

find_package(Catch2 REQUIRED)

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
target_link_libraries(asyop-tests PRIVATE Catch2::Catch2 asyop-asio)

include(Catch)
catch_discover_tests(asyop-tests)
