---
layout: default
title: Common
nav_order: 2
parent: Library description
---

## Common
The common part of the library describes primitives for everyday usage, presents the capabilities of the library and can be used as a reference of customization points for client code.

The headers that correspond to this section are completely optional but generally should not introduce collisions with the current codebase. They should be treated like convenience functions that promote a certain style of usage.

### `asy::op`
This is where it all begins :) The library provides a global function `asy::basic_op(...)` (and an alias `asy::op(...)` with default error type) that supports several types of input. In all cases, this function invokes an operation and returns a handle to it. There are four possible usage cases (`&&` here is a forward reference):
1. The `basic_op_handle<T,Err>` argument is simply forwarded as a return type.
    In this case, use the followng signature is: `basic_op_handle<T,Err> asy::basic_op(basic_op_handle<T,Err>&&)`.
2. If the first argument is a continuation, then the continuation is immediately invoked (with input arguments, if provided). 
    In this case, use the following signature is: `basic_op_handle<T,Err> asy::basic_op(Continuation&&, Args&&...)`.
3. If the single argument is not an operation handle or continuation, it is treated as an operation result that is already ready (type can be deduced or explicitly specified).
   In this case, use the followng signature is: `basic_op_handle<T,Err> asy::basic_op(T&&)`, i.e.: `asy::basic_op<std::error_code, int>(42)` or `asy::op(42)` or even `asy::op<double>(42)`.
4. If called with no arguments, the default-constructed result is immediately ready via returned operation handle. The type is specified explicitly: `asy::basic_op<std::error_code, int>()` or `asy::op<std::string>()`.

### Continuation signatures
Asy::op library implements several continuation types out-of-the-box, this section will describe each of them with a brief explanation. All of them target functor objects without requiring inheritance or any concrete type. To achieve that, the lightweight c++ concept-like implementation is used to select the specialization that is suitable for certain callable signature (arguments and return type). The current implementation uses the "concept" name similar to the future C++20 standard.

Following description will use `B = A.then(F)` example, where `A` is a `basic_op_handle<Input, IErr>`, `B` is a `basic_op_handle<Output, OErr>`.

#### Async continuation
This continuation specialization is used when the callable satisfies `AsyncContinuation` concept. Concept expects the functor to be compatible with `void foo(basic_context_ptr<Output, OErr>, Input&&)` for success path and `void foo(basic_context_ptr<Output, OErr>, IErr&&)` for failure path. The functor should return the result via its first argument. 

This concept has one caveat: due to limitations of the type deduction in C++, it is not possible to use a callable with templated or overloaded `operator()`.

#### Simple continuation
Simple continuation is best suited for non-async calls (they return its result via simple `operator()` invocation). Corresponding concept - `SimpleContinuation`, expected signature - `Output foo(Input&&)`. This type of continuation does not have a mechanism to return an error, thus always succeeds.

Simple continuation starts a family of a callable that returns a certain type. It is possible to define continuations that have the same input type as "simple" but are used if the return type matches a specific concept. When the concept is not matched, the default implementation will be used (simple continuation).

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
Asy::op library provides a set of functions that describe the parallel running of async operations and combining their results.

#### When any
This function runs a set of async operations that are passed in the argument list and returns a handle to the operation that returns the first result of the computation. The given operation describes a race between operations. When the first operation finishes (success or failure) - others are discarded and canceled. The output type (not a return type) of `basic_when_any<Err>(Op1, Op2, ...)` is `std::variant<Op1_Output, Op2_Output, ...>`. The requirement is that all operations have compatible error types. The cancellation of "when any" will cancel all its running operations.

#### When all
This function runs a set of sync operations that are passed in the argument list and returns when all of them are finished with a success or error. The output type of such operation is `std::tuple<std<variant<Op1_Output, Op1_Err>>, std<variant<Op2_Output, Op2_Err>>>`. The cancellation of "when all" will cancel all its running operations.

#### When success
This function is similar to "when all" but requires that all operations were successful. If any of the operations fails - others are discarded and canceled, the error object is forwarded to "when success" result. The output type if `basic_when_all()` is `std::tuple<Op1_output, Op2_Output, ...>`. The cancellation of "when success" will cancel all its running operations.
<!--stackedit_data:
eyJoaXN0b3J5IjpbLTg3Mzk1ODQ2MCwtMTM1Nzg0MzQwOV19
-->