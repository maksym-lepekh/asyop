# asy::op

[![](https://img.shields.io/github/license/maksym-lepekh/asyop.svg?style=flat-square)](https://github.com/maksym-lepekh/asyop/blob/master/LICENSE)
[![](https://img.shields.io/travis/com/maksym-lepekh/asyop/master.svg?style=flat-square&logo=travis-ci)](https://travis-ci.com/maksym-lepekh/asyop)
[![](https://img.shields.io/codecov/c/gh/maksym-lepekh/asyop.svg?logo=codecov&style=flat-square)](https://codecov.io/gh/maksym-lepekh/asyop)

C++17 library for asynchronous operations

## Features
* Provides abstraction for asynchronous operations and continuations
* Well-defined error handling model (inspired by Promises/A+), no exceptions needed
* Supports cancellation of continuation chains
* Can be optimized for single thread usage
* Non-intrusive: does not require to use specific error types, executors or handlers
* A lot of headers are optional: replace them with more project-specific variants
* Extendable:
  * Easy adaptation for any event loop or thread pool
  * Support of any error type
  * It is possible to declare customized handler type or callable signature
* Modern C++ (requires c++17), modern CMake
* Available in Conan
* Code quality is maintained by CI ([check here](https://travis-ci.com/maksym-lepekh/asyop))

## Docs

* [Getting started](https://maksym-lepekh.github.io/asyop/getting-started.html)

* [Library guide](https://maksym-lepekh.github.io/asyop/)

## Future

* Better documentation
* Exception-awareness, use of `noexcept`
* Use of customizable allocator
* Adapt to C++20: concepts, coroutine support, modules, ...
* Adapt to Executors proposal
* Fix MSVC build :)
