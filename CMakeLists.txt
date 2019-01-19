cmake_minimum_required(VERSION 3.9)

if (NOT DEFINED PROJECT_NAME)
    set(NOT_SUBDIR ON)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/libs/Catch2/contrib")

# project declaration
project(asyop LANGUAGES CXX VERSION 0.1)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# dependencies
add_subdirectory(libs/Catch2)
find_package(ASIO REQUIRED)

# main library
add_library(asyop SHARED src/asy.cpp)
target_include_directories(asyop PUBLIC include)

add_library(asyop-asio SHARED src_asio/event_loop.cpp)
target_include_directories(asyop-asio PUBLIC include_asio)
target_link_libraries(asyop-asio PUBLIC asyop ASIO::ASIO)


add_library(asyop::asyop ALIAS asyop)
add_library(asyop::asio ALIAS asyop-asio)


# standalone
if (NOT_SUBDIR)
    # tests
    add_executable(tests tests/main.cpp tests/type_traits.cpp)
    target_link_libraries(tests PRIVATE Catch2::Catch2 asyop-asio)

    # test driver
    enable_testing()
    include(Catch)
    catch_discover_tests(tests)
endif()
