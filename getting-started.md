---
layout: default
title: Getting started
nav_order: 2
---

# Getting started
The most easy way to use the library is to link it with `target_link_libraries(... asyop::asyop)` within the CMake project. It is possible to use the library without CMake, but the developer will be required to set all the flags by himself. Don't worry, this process is not so hard because the library doesn't use any `#define` or special treating.

## CMake subdirectory
One of the option is to download the library sources from github repo and somehow add it to project root (git submodule, ExternalProject_add, etc.).
In this case, the library can be added into client's project through CMake command: `add_subdirectory(${path_to_asyop}/lib)`. After that, client's project will be populated with a new target: `asyop::asyop`. The `CMakeLists.txt` file of the library is designed to detect the use of `add_subdirectory()` so it avoid polluting the client's project with unneccessary tests and other stuff.

## Package manager dependency
The asy::op library is available in Conan. While the library is in development stage, it is published in the separate repository, so in order to resolve the dependency, the user should run the following command in its machine:
```bash
conan remote add <REMOTE> https://api.bintray.com/conan/maxl/cpp
```
The library itself is currently named `asyop/<version>@maxl/testing`, use that string in yo

## Dependencies
TODO
<!--stackedit_data:
eyJoaXN0b3J5IjpbLTk5NzY5OTAzNSwtNDQ1MDYzNjI1XX0=
-->