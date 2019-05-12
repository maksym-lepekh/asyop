
# Introduction
The purpose of this library is to provide small, simple to use abstractions to write a program in asyncrchronous way with minimal dependency on concrete coroutine, thread pool or event loop implementation. The library is heavy templated and requires c++17 features support.

The librariry is desined with following goals in mind:
* Reduce dependencies on 3rd party libraries
* Do not pollute parent project with CMake
* Only modern techniques in c++ and cmake code are used
* Introduce easy way to adapt every callable ojbect into continuation
* Prevent thread syncronization where it is not needed
* Help with error handling but do not force specific error types, let user to use utils from its own codebase
* Provide meaningful support for cancellation
* Support different executors (thread pools, event loops, etc.) via ad-hoc interface, let user to write own library support
* Write reference implementations for asio, std::thread, but let user to replace it with different code

This library should utilize modern CI usage, such as multi-os, multi-toolchain cheks, code coverage reports, static analysis (TODO) and sanitizer (TODO) cheks.

# Getting started
TODO
## CMake subdirectory
TODO
## Dependencies
TODO

# Library description
## Core
### Operation handle
### Operation context
### Continuation concept
### Executor
## Common
### Continuation signatures
### Compound operations
## Default aliases
### std::error_code
## Customization points
### Custom callable signature
### Custom return type
### User-defined error type
<!--stackedit_data:
eyJoaXN0b3J5IjpbLTkzNjM4MDI3Nl19
-->