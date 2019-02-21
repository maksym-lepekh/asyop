cmake_minimum_required(VERSION 3.9)

if (NOT DEFINED PROJECT_NAME)
    set(NOT_SUBDIR ON)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")


# project declaration
project(asyop LANGUAGES CXX VERSION 0.1)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


# main library
add_library(asyop SHARED src/asy.cpp)
target_include_directories(asyop PUBLIC include)


# ASIO integration
find_package(ASIO)
if (ASIO_FOUND)
    add_library(asyop-asio SHARED src_asio/event_loop.cpp)
    target_include_directories(asyop-asio PUBLIC include_asio)
    target_link_libraries(asyop-asio PUBLIC asyop ASIO::ASIO)
endif()


if (NOT_SUBDIR)
    # standalone project, add tests
    find_package(Catch2 REQUIRED)
    find_package(ASIO REQUIRED)

    add_executable(tests
        tests/main.cpp
        tests/type_traits.cpp
        tests/continuation_info.cpp
        tests/then.cpp
        tests/op.cpp
        tests/ops.cpp
        tests/value_or_error.cpp
        tests/asio.cpp)
    target_link_libraries(tests PRIVATE Catch2::Catch2 asyop-asio)

    enable_testing()
    include(Catch)
    catch_discover_tests(tests)
else()
    # added via add_subdirectory, create imported targets
    add_library(asyop::asyop ALIAS asyop)
    if (ASIO_FOUND)
        add_library(asyop::asio ALIAS asyop-asio)
    endif()
endif()
