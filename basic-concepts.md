---
layout: default
title: Basic concepts
nav_order: 3
---

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
