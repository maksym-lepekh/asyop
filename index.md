
---
layout: home
title: Introduction
nav_order: 1
---

# Introduction
The purpose of this library is to provide small, simple to use abstractions to write a program in an asynchronous way with minimal dependency on concrete coroutine, thread pool or event loop implementation. The library is heavy templated and requires c++17 features support.

The library is designed with the following goals in mind:
* Reduce dependencies on 3rd party libraries
* Do not pollute parent project with CMake
* Only modern techniques in c++ and Cmake code are used
* Introduce an easy way to adapt every callable object into continuation
* Prevent thread synchronization where it is not needed
* Help with error handling but do not force specific error types, let the user use utils from its own codebase
* Provide meaningful support for cancellation
* Support different executors (thread pools, event loops, etc.) via an ad-hoc interface, let the user write own library support
* Write reference implementations for Asio, std::thread, but let the user replace it with different code

This piece of code should utilize modern CI usages, such as multi-os, multi-toolchain checks, code coverage reports, static analysis (TODO) and sanitizer (TODO) cheks.
<!--stackedit_data:
eyJoaXN0b3J5IjpbMTk4OTE2ODY1MF19
-->