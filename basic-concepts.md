---
layout: default
title: Basic concepts
nav_order: 3
---

# Basic concepts

## Operations
Operation is a procedure that takes some input and produces an output. The operation can fail thus producing "error description" instead of expected data.
Input can be a single argument, multiple arguments or nothing (`void`).
The output is a single value, error description or nothing.

Operation is permitted to produce output after returning control to the caller. Such an operation is called an asynchronous operation. An event of emitting output is called "asynchronous return".

## Continuations
Continuation B for some operation A is a function that is called immediately after A is done. Operation A is considered done after it returns the value, error or nothing. Then, continuation B "continues" processing with data that was returned from operation A.
Continuation is also an operation, thus it can have own continuation. That gives an opportunity to declare chains and process data with several reusable steps.
Such chains support splits and join to describe parallel computation. Data processing can be depicted as a graph where nodes represent each operation.

## Error handling
When an error occurs, it is passed as a result of the asynchronous operation. Then, the corresponding error-handling continuation is invoked. It is not available, it is forwarded to the next continuation pair until the error is consumed or left pending at the end of the continuation chain. If the error is consumed, the computation goes back to the success path of execution, thus invoking the success continuation that receives `void`.

## Cancelation
Cancelation is a premature failure of the current operation with a special "canceled" error object that results in the invocation of error handling path in the continuation chain.
<!--stackedit_data:
eyJoaXN0b3J5IjpbLTIwNjA2NDU3MjksODc4OTQzNTczXX0=
-->