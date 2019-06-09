
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

This peace of code should utilize modern CI usage, such as multi-os, multi-toolchain cheks, code coverage reports, static analysis (TODO) and sanitizer (TODO) cheks.

# Getting started
TODO

## CMake subdirectory
TODO

## Dependencies
TODO

# Basic concepts

## Operations
Operation is a procedure that takes some input and produces an output. Operation can fail thus producing "error description" instead of expected data.
Input can be a single argument, multiple arguments or nothing (`void`).
Output is a single value, error description or nothing.

Operation is permitted to produce output after the returning control to the caller. Such operation is called an asynchronous operation. An event of emitting an output is called "asynchronous return".

## Continuations
Continuation B for some operation A is a function that is called immediately after A is done. Operation A is considered done after it returns value, error or nothing. Then, continuation B "continues" processing with data that was returned from operation A.
Continuation is also an operation, thus it can has own continuation. That  gives an opportunity to declare chains  and process data with several several reusable steps.
Such chains support splits and joins to describe parallel computation. Data processing can be depicted as a graph where nodes represent each operation.

## Error handling
TODO

## Cancellation
TODO

# Library description
This section contains brief explanation of all available types and functions that are provided by this library. Library is divided by header file structure: some headers are required and other are optional and can be easily replaced if they have conflicts with the project. It gives flexibility for library adoptions and implements one of the design goals.

## Core
Core part of the library describes fundamental types and facilities that a bare minimum for implementation of the asynchronous operations and continuations. It has minimal dependency, does not have any concrete type for error object or event loop preference, so it can be usable within any project without any collisions.

Library's core describes trio of the library foundation: `op_handle` - public part of async operation, `context` - private part of async operation and `executor` - global entity, that is capable of running client's functors.

### Operation handle
An asynchronous operation is represented as an object of type `op_handle`. This handle corresponds to an operation that is already running, has pending results or fully completed. By design, there can be no be a valid handle that designates the operation that is not started yet. This fact reduces the implementation complexity and a range of possible errors and race conditions.

Operation handle has "weak pointer" semantics regarding to operation itself. It does not affect operation lifetime, thus can be safely destroyed when not needed. There is a limited set of available public methods which include: "cancel", "set continuation" and "abort" (**TODO**).

#### Setting the continuation
There are several methods that can be used to specify the continuation for the operation. They are: `.then(SuccessCb&&)`, `.on_failure(FailureCB&&)` and `.then(SuccessCb&&, FailureCb&&)`, use them to specify what to do if operation succeded or fails. These methods are templates and accept a wide range of possible functor types. All possible function signatures will be described later in this document (see Continuation signatures and Customization points - Custom callable signature). For now, it is worth mentioning that continuations are asynchronous operations too. 

The calls  `A.then(B)` and `A.on_failure(B)` return objects of type `op_handle` representing the ongoing operation that consists of two steps: "wait until A is finished" and "call B". The difference between them is that B is called only if the execution goes in the right path:
* `A.then(B)`: B is called only if A succedes
* `A.on_failure(B)`: B is called only if A fails
* `A.then(B, C)`: B is called if A succedes, C is called if A fails

Operations create parent-child connections when `.then()` and similar is used. It gives the opportunity to implement more sophisticated cancellation mechanims.

#### Cancellation
Cancellation is an event of premature operation failure with specific "operation cancelled" error object. It will start "failure" execution path in continuations, thus `on_falure` continuations will be invoked. This mechanism allows proper cleanup or other kind of gracefull shutdown of processing. Mentioned error object depends on current `Err` type an is described more in Customization points - User-defined error type section.

Method `.cancel()` can only have effect if the corresponding operation is not finished yet or it is finished, but there were no continuations invoked. In the latter case, the operation result is replaced with the "operation cancelled" error object. Future continuations will be immediately invoked with a failure "path".

If the operation has a parent that is not finished yet, the parent will be cancelled instead. Thus, failure path will be executed earlier. This process is recursive and ends when already finished parent was found. That means only last operation handle is needed to cancel a whole continuation chain as early as possible.

#### Operation abortion
TBD

### Operation context
Operation context `basic_context<T, Err>` is a special type that holds current state of the asynchronous operation. It is also used as a container for pending continuations or operation result.

The only interaction of client code with this class is performed inside a client's functor that is used as an asynchronous operation. It is available when the functor satisfies `AsyncContinuation` concept (has a function signature similar to `void foo(context_ptr<T, Err>, Input&&...)`, where `context_ptr<>` is an alias for `std::shared_ptr<basic_context<>>`). 
In this case, clients functor must call one of the context methods to notify that operation is completed: `async_return(Ret&&)`, `async_success(T&&)` or `async_failure(Err&&)`. The first one will select "success" or "failure" depending on the arguments type, other two can be used to disambiguate the first one (or to be more explicit).

Context class uses global executor to call continuations and to perform thread safety locks (if needed). This process will be discussed later. On the other hand, context is referenced internally by operation handle and holds the actual implementation of cancellation and "setting the continuation".

### Continuation concept
The methods `basic_op_handle::then()` and `basic_op_handle::on_failure()` are templates and can be called with any type. The core library does not contain the information about how the continuation should be called, how to interpret returned object and how to differentiate between success or failure. 

It is expected that user provides a specialization to `struct continuation<>` that defines two static public methods, thus making given functor type "compatible" with a `asy::op` library. These methods are `to_handle(F, Args...)` and `deferred(Ctx, F, Args)`. The former must call the functor with giver args and return operation handle that corresponds to the operation which "returns" immediately when user function is finished. The latter is similar, but instead of operation handle, it should forward the functor results via pointer to async context.

Common part of the asy::op contains a set of specializations for the `struct continuation<>` that should cover most of the cases, thus can be used as quick start for new projects. They will be covered later.

### Executor
The global executor is used to connect the core library with the preferred execution model of the clients project. In other words, the executor is used as a wrapper for event loop or any thread pool.

The asy::op library contains reference implementation of Asio `io_service` support. It uses the `executor` internally to set everything up. It is expected that other integrations will hide executor usage in the same maner, so end user interaction with `class executor` is minimal.

Anyway, the executor is implemented as a singleton and has following public methods: `schedule_execution(F, TID)`, `should_sync(TID)` and `set_impl(TID, F, bool should_sync)`.  The first one is used internally by `basic_context<>` to run the continuation on the preferred thread. Most cases it'll be a current thread. The second one, `should_sync()` is also used by `basic_context<>` to check if the mutexes should be used when calling `.cance()`, `async_return()`, etc. The executor returns the boolean depending on the current setup of event loops or thread pools and their preferences. The third method `set_impl()` is used register certain execution implementation (thread, thread pool, event loop) for the specified thread. This is the main point of connection between asy::op and other libraries. This method has a boolean arg to notifiy the executor that the code is running in a multithreaded environment and thread safety mechanisms should be employed.

The asy::op supports running several event loops and thread pools each on its own thread. The async operation chains can be isolated within the same event loop or can be balanced between threads, but this is fully up to user's choice. The actual balancer is implemented by client's code and is set via `set_impl()` method for each thread separately. Continuations are called with the preferred thread that equals parent's execution thread. The balancer of the preferred thread can reschedule the continuation on the other one. Please note that asy::op does not implelent balancing, it only provides the compatible interface ;)

## Common
The common part of the library describes primitives for every day usage, presents the capabilities of the library and can be used as a reference of customization points for client code.

The headers that correspond to this section are completely optional but generally should not introduce collisions with current codebase. They should be treated like convenience functions that promote certain style of the usage.

### `asy::op`
This is where it all begins :) The library provides a global function `asy::basic_op(...)` (and an alias `asy::op(...)` with default error type) that supports several types of input. In all cases, this function invokes an operation and returns a handle to it. There are four possible usage cases (`&&` here is a forward reference):
1. The `basic_op_handle<T,Err>` argument is simply forwarded as a return type.
    In this case, use the followng signature is: `basic_op_handle<T,Err> asy::basic_op(basic_op_handle<T,Err>&&)`.
2. If the first argument is a continuation, then the continuation is immediately invoked (with input arguments, if provided). 
    In this case, use the followng signature is: `basic_op_handle<T,Err> asy::basic_op(Continuation&&, Args&&...)`.
3. If the single argumet is not an operation handle or continuation, it is treated as an operation result that is alredy ready (type can be deduced or explicitly specified).
   In this case, use the followng signature is: `basic_op_handle<T,Err> asy::basic_op(T&&)`, i.e.: `asy::basic_op<std::error_code, int>(42)` or `asy::op(42)` or even `asy::op<double>(42)`.
4. If called with no arguments, the default-constructed result is immediately ready via returned operation handle. The type is specified explicitly: `asy::basic_op<std::error_code, int>()` or `asy::op<std::string>()`.

### Continuation signatures
Asy::op library implementes several continuation types out-of-the-box, this section will describe each of them with breif explanation. All of them target functor objects without requiring inheritance or  any concrete type. To achive that, the lightweigth c++ concept-like imlementation is used to select the specialization that is suitable for certain callable signature (arguments and return type). The current implementation uses "concept" name similar to the future C++20 standard.

Following description will use `B = A.then(F)` example, where `A` is a `basic_op_handle<Input, IErr>`, `B` is a `basic_op_handle<Output, OErr>`.

#### Async continuation
This continuation specialization is used when the callable satisfies `AsyncContinuation` concept. Concepts expects the functor to be compatible with `void foo(basic_context_ptr<Output, OErr>, Input&&)` for success path and `void foo(basic_context_ptr<Output, OErr>, IErr&&)` for failure path. The functor should return the result via its first argument. 

This concept has one caveat: due to limitations of the type deduction in C++, it is not possible to use a callable with templated or overloaded `operator()`.

#### Simple continuation
Simple continuation is best suited for non-async calls (they return its result via simple `operator()` invocation). Corresponding concept - `SimpleContinuation`, expected signature - `Output foo(Input&&)`. This type of continuation does not have a mechinism to return error, thus always succeedes.

Simple continuation starts a family of callable that return certain type. It is possible to define continuations that have same input type as "simple" but are used if the retunt type matches specific concept. When concept is not matched, default implementation will be used (simple continuation).

#### ARet continuation
Part of simple continuation family, this is a short name of "async op handle returning continuation". The corresponding concept: `ARetContinuation`, expected signature: `basic_op_handle<Output, OErr> foo(Input&&)` for success and `basic_op_handle<Output, OErr> foo(IErr&&)` for failure. Unlike the simple continuation, the operation is "returns" when returned operation is "returns".

#### ValueOrError continuation
Also a part of simple continuation family. It is used when a return type satisfies `ValueOrError` concept. The concept itself requires: non-void `.value()`, non-void `.error()`, boolean `.has_value()`.

#### ValueOrNone continuation
Very similar to ValueOrError continuation, uses another concept: `ValueOrNone`. This concept requires: non-void `.value()`, absence of `.error()`, boolean `.has_value()`.

Note that this concept matches `std::optional<Output>`.

#### NoneOrError continuation
Same as previous two, uses `NoneOrError` concept. The concept requires: non-void `.error()`, boolean `.has_value()`, absence of non-void `.value()`.

### Compound operations
Asy::op libarty provides a set of functions that describe parallel running of async operations and combining their results.

#### When any
This function runs a set of async operations that are passed in argument list and returns a handle to the operation that returns first result of computation. Given operation describes a race between operations. When the first operation finishes (success or failure) - other are discarded and cancelled. The output type (not a return type) of `basic_when_any<Err>(Op1, Op2, ...)` is `std::variant<Op1_Output, Op2_Output, ...>`. The requirement is that all operation have compatible error type. The cancellation of "when any" will cancel all the its running operations.

#### When all
This function runs a set of sync operations that are passed in argument list and returns when all of them are finished with a success or error. The output type of such operation is `std::tuple<std<variant<Op1_Output, Op1_Err>>, std<variant<Op2_Output, Op2_Err>>>`. The cancellation of "when all" will cancel all the its running operations.

#### When success
This function is similar to "when all" but requires that all operations were successfull. If any of the operations fails - others are discarded and cancelled, the error object is forwarded to "when success" result. The output type if `basic_when_all()` is `std::tuple<Op1_output, Op2_Output, ...>`. The cancellation of "when success" will cancel all the its running operations.

## Customization points
On of the library desing goas is to provide maximum flexibility and customization for end user. This should reduce the adoption cost and make the library less intrusive. 

### User-defined error type
As you could observe before, most of the functions and types are templates that depend on two main arguments: `T` - return type and `Err` - error type that describes execution failure. To reduce the typing effort, one can create template aliases with predefined error types such as `int`, `boost::error_code`, `std::outcome`, etc. The asy::op library provides the aliases for most of the templates with predefined `std::error_code` as the most standard error type in the modern C++. Most of the times, default aliases have the name of the general tempalte without `basic_` prefix: `op_handle<T> -> basic_op_handle<T, std::error_code>`, `asy::context<T> -> asy::basic_context<T, std::error_code>`, `asy::when_all() -> asy::basic_when_all<std::error_code>`, etc.

Please note that default aliases are declared in optional header, so it is completely fine to replace them with own custom header. Another way to avoid the collisions is to use another namespace ;)

### Custom callable signature
As was described before, the library supports user defined continuation types. To make it work, the developer should declare a specialisation for `struct asy::continuation<UserType>` with certain methods (the details were described before). If you plan to mix user-defined callables with the continuation specializations from asy::op library, please make sure that user type does not satisfy library-defined concepts: wierd compiler erros are expected otherwise.

The user also has a possibility to describe custom callable type in the same manner as asy::op library - using pseudo-concetps or any other mothod that involves SFINAE technique, the `struct asy::continuation` is SFINAE-ready.

### Custom return type
The paragraph above describes the case when callable type differs from out-of-the box support provided by asy::op library. The other option is to use a usual functor that returns some user-defined type of object. In that case, the user should create a specialization for `struct asy::simple_continuation<>` from asy/common/simple_continuation.hpp. Same rules apply as before: specialize for a concrete type or play with concepts and SFINAE.

# Further reaing
This guide provide brief explanation of the available functions, types and techniques. (I hope it will be expanded with more details and examples in future). For more information, please read doxygen comments and sources of unit-tests.
<!--stackedit_data:
eyJoaXN0b3J5IjpbLTE5OTM5NjEzNTIsLTkzNjM4MDI3Nl19
-->