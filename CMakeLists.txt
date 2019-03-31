cmake_minimum_required(VERSION 3.9)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# detect add_subdirectory
if (NOT DEFINED PROJECT_NAME)
    set(NOT_SUBDIR ON)
endif()

# project declaration
project(asyop LANGUAGES CXX VERSION 0.1)
find_package(Threads REQUIRED)


# main library
add_library(asyop SHARED src/asy.cpp)
target_include_directories(asyop PUBLIC include)
target_link_libraries(asyop PUBLIC Threads::Threads)
target_compile_features(asyop PUBLIC cxx_std_17)


# ASIO integration
find_package(ASIO)
if (ASIO_FOUND)
    add_library(asyop-asio SHARED src_asio/evloop_asio.cpp)
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
        tests/continuation.cpp
        tests/then.cpp
        tests/op.cpp
        tests/ops.cpp
        tests/value_or_error.cpp
        tests/asio.cpp
        tests/thread.cpp)
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
