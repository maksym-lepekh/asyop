cmake_minimum_required(VERSION 3.9)

set(CMAKE_C_FLAGS_COVERAGE "-O0 -g --coverage")
set(CMAKE_CXX_FLAGS_COVERAGE "-O0 -g --coverage")
set(CMAKE_SHARED_LINKER_FLAGS_COVERAGE "--coverage")
set(CMAKE_STATIC_LINKER_FLAGS_COVERAGE "--coverage")
set(CMAKE_EXE_LINKER_FLAGS_COVERAGE "--coverage")

set(CMAKE_C_FLAGS_AUBSAN "-O1 -g -fsanitize=address -fsanitize=undefined,integer,nullability -fno-omit-frame-pointer -fsanitize-address-use-after-scope -fsanitize-recover=address")
set(CMAKE_CXX_FLAGS_AUBSAN "-O1 -g -fsanitize=address -fsanitize=undefined,integer,nullability -fno-omit-frame-pointer -fsanitize-address-use-after-scope -fsanitize-recover=address")
set(CMAKE_SHARED_LINKER_FLAGS_AUBSAN "-fsanitize=address -fsanitize=undefined,integer,nullability")
set(CMAKE_STATIC_LINKER_FLAGS_AUBSAN "-fsanitize=address -fsanitize=undefined,integer,nullability")
set(CMAKE_EXE_LINKER_FLAGS_AUBSAN "-fsanitize=address -fsanitize=undefined,integer,nullability")
# ASAN_OPTIONS=detect_stack_use_after_return=1:halt_on_error=false:detect_leaks=1

set(CMAKE_C_FLAGS_TSAN "-O1 -g -fsanitize=thread")
set(CMAKE_CXX_FLAGS_TSAN "-O1 -g -fsanitize=thread")
set(CMAKE_SHARED_LINKER_FLAGS_TSAN "-fsanitize=thread")
set(CMAKE_STATIC_LINKER_FLAGS_TSAN "-fsanitize=thread")
set(CMAKE_EXE_LINKER_FLAGS_TSAN "-fsanitize=thread")
# TSAN_OPTIONS=

set(CMAKE_C_FLAGS_MSAN "-O1 -g -fsanitize=memory -fno-omit-frame-pointer -fsanitize-memory-track-origins")
set(CMAKE_CXX_FLAGS_MSAN "-O1 -g -fsanitize=memory -fno-omit-frame-pointer -fsanitize-memory-track-origins")
set(CMAKE_SHARED_LINKER_FLAGS_MSAN "-fsanitize=memory")
set(CMAKE_STATIC_LINKER_FLAGS_MSAN "-fsanitize=memory")
set(CMAKE_EXE_LINKER_FLAGS_MSAN "-fsanitize=memory")
# MSAN_OPTIONS=
