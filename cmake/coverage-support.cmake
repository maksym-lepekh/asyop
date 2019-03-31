cmake_minimum_required(VERSION 3.9)

set(CMAKE_C_FLAGS_COVERAGE "-O0 -g --coverage")
set(CMAKE_CXX_FLAGS_COVERAGE "-O0 -g --coverage")
set(CMAKE_SHARED_LINKER_FLAGS_COVERAGE "--coverage")
set(CMAKE_STATIC_LINKER_FLAGS_COVERAGE "--coverage")
set(CMAKE_EXE_LINKER_FLAGS_COVERAGE "--coverage")
