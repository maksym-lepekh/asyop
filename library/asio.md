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
The asy::op core library requires to setup executor in order to run the async operations. For this purpose, `asyop::asio` provides two global functions: `asy::this_thread::set_event_loop(io_service&)` and `asy::this_thread::get_event_loop`. These functions do all the work and expect a single-thread execution environment: program uses single event loop 

## Asiofy

## Sleep

<!--stackedit_data:
eyJoaXN0b3J5IjpbMTIwOTMxMzQ4LC0yMDk1NDAxMzEzXX0=
-->