---
layout: default
title: Core
nav_order: 1
parent: Library description
---

## Core
The core part of the library describes fundamental types and facilities that are a bare minimum for implementation of the asynchronous operations and continuations. It has a minimal dependency, does not have any concrete type for error object or event loop preference, so it can be used within any project without any collisions.

Library's core describes the trio of the library foundation: `op_handle` - public part of the async operation, `context` - private part of the async operation and `executor` - global entity, that is capable of running client's functors.

### Operation handle
Asynchronous operation is represented as an object of type `op_handle`. This handle corresponds to an operation that is already running, has pending results or fully completed. By design, there can not be a valid handle that designates the operation that is not started yet. This fact reduces the implementation complexity and a range of possible errors and race conditions.

Operation handle has "weak pointer" semantics regarding operation itself. It does not affect operation lifetime, thus it can be safely destroyed when not needed. There is a limited set of available public methods which include: "cancel", "set continuation" and "abort".

#### Setting the continuation
There are several methods that can be used to specify the continuation of the operation. They are: `.then(SuccessCb&&)`, `.on_failure(FailureCB&&)` and `.then(SuccessCb&&, FailureCb&&)`, use them to specify what to do if the operation succeeded or fails. These methods are templates and accept a wide range of possible functor types. All possible function signatures will be described later in this document (see [Continuation signatures](core.md#continuation-concept) and [Customization points - Custom callable signature](customization.md#custom-callable-signature)). For now, it is worth mentioning that continuations are asynchronous operations too. 

The calls  `A.then(B)` and `A.on_failure(B)` return objects of type `op_handle` representing the ongoing operation that consists of two steps: "wait until A is finished" and "call B". The difference between them is that B is called only if the execution goes in the right path:
* `A.then(B)`: B is called only if A succeeds
* `A.on_failure(B)`: B is called only if A fails
* `A.then(B, C)`: B is called if A succeeds, C is called if A fails

Operations create parent-child connections when `.then()` and similar is used. It gives the opportunity to implement a more sophisticated cancellation mechanism.

#### Cancellation
Cancellation is an event of premature operation failure with a specific "operation canceled" error object. It will start a "failure" execution path in continuations, thus `on_falure` continuations will be invoked. This mechanism allows proper cleanup or another kind of graceful shutdown of processing. The mentioned error object depends on the current `Err` type an is described more in Customization points - User-defined error type section.

Method `.cancel()` can only have an effect if the corresponding operation is not finished yet or it is finished, but there were no continuations invoked. In the latter case, the operation result is replaced with the "operation canceled" error object. Future continuations will be immediately invoked with a failure "path".

If the operation has a parent that is not finished yet, the parent will be canceled instead. Thus, the failure path will be executed earlier. This process is recursive and ends when an already finished parent was found. That means only the last operation handle is needed to cancel a whole continuation chain as early as possible.

#### Operation abortion
Abort can be used to prematurely discard the operation result and prevent invocation of all continuations.

Method `.abort()` can only have an effect if the corresponding operation is not finished yet or it is finished, but there were no continuations invoked.

### Operation context
Operation context `basic_context<T, Err>` is a special type that holds the current state of the asynchronous operation. It is also used as a container for pending continuations or operation result data.

The only interaction of client code with this class is performed inside a client's functor that is used as an asynchronous operation. It is available when the functor satisfies `AsyncContinuation` concept (it has a function signature similar to `void foo(context_ptr<T, Err>, Input&&...)`, where `context_ptr<>` is an alias for `std::shared_ptr<basic_context<>>`). 
In this case, clients functor must call one of the context methods to notify that operation is completed: `async_return(Ret&&)`, `async_success(T&&)` or `async_failure(Err&&)`. The first one will select "success" or "failure" depending on the argument's type, the other two can be used to disambiguate the first one (or to be more explicit).

Context class uses global executor to call continuations and to perform thread safety locks (if needed). This process will be discussed later. On the other hand, context is referenced internally by operation handle and holds the actual implementation of cancellation and "setting the continuation".

### Continuation concept
The methods `basic_op_handle::then()` and `basic_op_handle::on_failure()` are templates and can be called with any type. The core library does not contain the information about how the continuation should be called, how to interpret returned object and how to differentiate between success or failure. 

It is expected that the user provides a specialization to `struct continuation<>` that defines two static public methods, thus making given functor type "compatible" with an `asy::op` library. These methods are `to_handle(F, Args...)` and `deferred(Ctx, F, Args)`. The former must call the functor with giver args and return the operation handle that corresponds to the operation which "returns" immediately when the user function is finished. The latter is similar, but instead of operation handle, it should forward the functor results via a pointer to the async context.

The common part of the asy::op contains a set of specializations for the `struct continuation<>` that should cover most of the cases, this can be used as a quick start for new projects. They will be covered [later](customization.md#custom-callable-signature).

### Executor
The global executor is used to connect the core library with the preferred execution model of the client's project. In other words, the executor is used as a wrapper for event loop or any thread pool.

The asy::op library contains [reference implementation](asio.md) of Asio `io_service` support. It uses the `executor` internally to set everything up. It is expected that other integrations will hide executor usage in the same manner, so end-user interaction with `class executor` is minimal.

Anyway, the executor is implemented as a singleton and has following public methods: `schedule_execution(F, TID)`, `should_sync(TID)` and `set_impl(TID, F, bool should_sync)`.  The first one is used internally by `basic_context<>` to run the continuation on the preferred thread. In most cases, it'll be a current thread. The second one, `should_sync()` is also used by `basic_context<>` to check if the mutexes should be used when calling `.cance()`, `async_return()`, etc. The executor returns the boolean depending on the current setup of event loops or thread pools and their preferences. The third method `set_impl()` is used to register certain execution implementation (thread, thread pool, event loop) for the specified thread. This is the main point of connection between asy::op and other libraries. This method has a boolean arg to notify the executor that the code is running in a multithreaded environment and thread safety mechanisms should be employed.

The asy::op supports running several event loops and thread pools each on its own thread. The async operation chains can be isolated within the same event loop or can be balanced between threads, but this is fully up to the user's choice. The actual balancer is implemented by the client's code and is set via `set_impl()` method for each thread separately. Continuations are called with the preferred thread that equals parent's execution thread. The balancer of the preferred thread can reschedule the continuation on the other one. Please note that asy::op does not implement balancing, it only provides the compatible interface ;)
<!--stackedit_data:
eyJoaXN0b3J5IjpbLTE3NDkxNDU0NywxMjkxNDY3NTcxLC05MT
U1NTE2NDNdfQ==
-->