---
layout: default
title: Thread support
nav_order: 4
parent: Library description
---

# Thread support
Asy::op is thread-aware and will link with `Threads::Threads` when used in CMake. Otherwise, it must be linked with `pthread` or similar. For now, thread support is limited to a thread -> async operation conversions. Hopefully, it will be extended in future :) Thread support is available in `asy/thread.hpp` header.

## Asyfy
The asy::op library provides a helper (`asy::fy()`) to quickly convert a blocking call into an asynchronous operation. The implementation isn't very efficient because a separate thread is created for each `asy::fy` call.
The function supports two types of input arguments. 

The first one: functor that calls blocking functions. It is invoked in a separate thread and the return type is used as an output type of the operation handle.

The second one: an instance of the `std::future<Output>`. The implementation will call `.get()` in a separate thread and will forward the output into async operation continuation.

There is an overload of `asy::fy()` that has  `std::thread` as an out parameter. This enables the client code to receive a handle to a newly created thread so the user can wait (`join()`) for it for proper destruction procedure. The thread will be detached otherwise.
<!--stackedit_data:
eyJoaXN0b3J5IjpbMTgzMjkwNDA2MiwxNDU3NzUxNDI5XX0=
-->