---
layout: default
title: ASIO integration
nav_order: 5
parent: Library description
---
# ASIO integration
The asy::op project provide a reference implementation of integration with ASIO in a form of separete `asyop::asio` library. As it was stated in Getting started section, it is conditionally available when `add_subdirectory()` and always avaliable using `find_package()`.

The `asyop::asio` library is not intrusive and designed to provide quick start helpers for asy::op users.

## Setup the executor
The asy::op core library requires to setup executor in order to run the async operations. For this purpose, `asyop::asio` provides two global functions: `asy::this_thread::set_event_loop(io_service&)` and `asy::this_thread::get_event_loop`. These functions do all the work and expect a single-thread execution environment: program uses single event loop thread and doesn't need synchronization.

## Asiofy
Asy::op's integration with ASIO provides two ways to convert ASIO's async operation into the asy::op's one.

The first method, `asy::asio::fy<Output...>(caller)` is usable with any method that requires completion handle and outputs `asio::error_code` plus other values (if availalbe). This adaptation method requires the client to specify expected handler arguments (without first error code),  then the client must provide a functor which first argument is a proper completion type (it is recommended to make the `operator()` a template). Inside the functor, the client should call the asio async method and  forward the completion handler.  

Example:  
```cpp
auto op_handle = asy::asio::fy<std::string>([&](auto completion_handler){  
   socket.async_read(std::move(completion_handler));  
});  
 
// decltype(op_handle) -> asy::basic_op_handle<std::string, asio::error_code>;  
```

The second method utilizes ASIO's support to customize return type of the async operation. It is available to all methods with proper implementation. The client is exepected to pass a special token `asy::asio::adapt` instead of completion handler. After that, the async operation will return perfectly usable `asy::basic_op_handle<Output, asio::error_code>`. Depending on number of ouput values, the `Output` and be a `void`, single type or `std::tuple<...>`.

## Sleep
`asyop::asio` provides a `sleep()` function that takes a `chrono::duration` as an arguments and uses default `io_service` for the current

## Operation with timeout
<!--stackedit_data:
eyJoaXN0b3J5IjpbMzkxMzczMjgwLC0yMDk1NDAxMzEzXX0=
-->