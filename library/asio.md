---
layout: default
title: ASIO integration
nav_order: 5
parent: Library description
---
# ASIO integration
The asy::op project provides a reference implementation of integration with ASIO in a form of separate `asyop::asio` library. As it was stated in the Getting started section, it is conditionally available when `add_subdirectory()` and always available using `find_package()`.

The `asyop::asio` library is not intrusive and designed to provide quick start helpers for asy::op users.

## Setup the executor
The asy::op core library requires to setup executor in order to run the async operations. For this purpose, `asyop::asio` provides two global functions: `asy::this_thread::set_event_loop(io_service&)` and `asy::this_thread::get_event_loop`. These functions do all the work and expect a single-thread execution environment: program uses single event loop thread and doesn't need synchronization.

## Asiofy
Asy::op's integration with ASIO provides two ways to convert ASIO's async operation into the asy::op's one.

The first method, `asy::asio::fy<Output...>(caller)` is usable with any method that requires completion handle and outputs `asio::error_code` plus other values (if available). This adaptation method requires the client to specify expected handler arguments (without first error code),  then the client must provide a functor which the first argument is a proper completion type (it is recommended to make the `operator()` a template). Inside the functor, the client should call the ASIO async method and forward the completion handler.  

Example:  
```cpp
auto op_handle = asy::asio::fy<std::string>([&](auto completion_handler){  
   socket.async_read(std::move(completion_handler));  
});  
 
// decltype(op_handle) -> asy::basic_op_handle<std::string, asio::error_code>;  
```

The second method utilizes ASIO's support to customize the return type of the async operation. It is available to all methods with proper implementation. The client is expected to pass a special token `asy::asio::adapt` instead of completion handler. After that, the async operation will return perfectly usable `asy::basic_op_handle<Output, asio::error_code>`. Depending on a number of output values, the `Output` and be a `void`, single type or `std::tuple<...>`.

## Sleep
`asyop::asio` provides a `sleep()` function that takes a `chrono::duration` as an arguments and uses default `io_service` for the current thread.

## Operation with timeout
ASIO integration declares an easy way to convert any async operation into the operation with a timeout. Internally it is a combination for `when_any()` with user-specified operation and `asy::asio::sleep`. The first finished operation cancels the other one. The output type is the same as in user-specified operation. The user's operation is converted into `asy::op_handle` using `asy::op()` and must have compatible with `asio::error_code` error type.
<!--stackedit_data:
eyJoaXN0b3J5IjpbLTEwNzI5NjM3NzgsLTExMzI0OTQ3NTEsLT
IwOTU0MDEzMTNdfQ==
-->