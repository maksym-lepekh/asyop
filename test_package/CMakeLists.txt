cmake_minimum_required(VERSION 3.9)

project(asyop-pkg-test LANGUAGES CXX)

include(${CMAKE_CURRENT_BINARY_DIR}/conan_paths.cmake OPTIONAL)
find_package(asyop REQUIRED)
find_package(Threads REQUIRED)

add_executable(tests
    main.cpp)

target_link_libraries(tests PRIVATE asyop::asyop)
