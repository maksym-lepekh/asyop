---
layout: default
title: Thread support
nav_order: 4
parent: Library description
---

# Thread support
Asy::op is thread-aware and will link with `Threads::Thereads` when used in CMake. Otherwise, it must be linked with `pthread` or similar. For now thread support is limited to a thread -> async operation convertions. Hopefuly, it will be extended in future :)

## Threadify
The asy::op library provides a helper (`asy::fy()`) to quickly convert a blocking call into an asynchronous operation. The implementation is very efficient because a separate thread is created for each `asy::fy` call.
The function supports two types of input arguments. The first one: functor that calls blocking 
<!--stackedit_data:
eyJoaXN0b3J5IjpbLTkwNDE2NzM4MF19
-->