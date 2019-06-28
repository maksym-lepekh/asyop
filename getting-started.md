
---
layout: default
title: Getting started
nav_order: 2
---

# Getting started
The easiest way to use the library is to link it with `target_link_libraries(... asyop::asyop)` within the CMake project. It is possible to use the library without CMake, but the developer will be required to set all the flags by himself. Don't worry, this process is not so hard because the library doesn't use any `#define` or special treating.

## CMake subdirectory
One of the options is to download the library sources from Github repo and somehow add it to project root (git submodule, ExternalProject_add, etc.).
In this case, the library can be added into the client's project through CMake command: `add_subdirectory(${path_to_asyop}/lib)`. After that, the client's project will be populated with a new target: `asyop::asyop`. The `CMakeLists.txt` file of the library is designed to detect the use of `add_subdirectory()` so it avoids polluting the client's project with unnecessary tests and other stuff.

If `find_package()` can successfully find Asio library, the new target will be added into the project: `asyop::asio`. It contains the reference implementation of Asio support.

## Package manager dependency
The asy::op library is available in Conan. While the library is in the development stage, it is published in the separate repository, so in order to resolve the dependency, the user should run the following command in its machine:
```bash
conan remote add <REMOTE> https://api.bintray.com/conan/maxl/cpp
```
The library itself is currently named `asyop/<version>@maxl/testing`, use that string in your Conan file in the `requires` section. 

Please note, when used with conan, asy::op has a transitive dependency of Asio library for `asyop::asio` target. To disable that, the package has the following option: `asio_support:False`.

After all Conan dependencies are installed, the user is expected to use `find_package(asyop)` to add the library into the project.

<!--stackedit_data:
eyJoaXN0b3J5IjpbMTIzMTY5NjQ0LC0xMDA3MjI2OTg4LC00ND
UwNjM2MjVdfQ==
-->