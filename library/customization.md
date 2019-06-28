
---
layout: default
title: Customization points
nav_order: 3
parent: Library description
---

## Customization points
One of the library design goas is to provide maximum flexibility and customization for the end user. This should reduce the adoption cost and make the library less intrusive. 

### User-defined error type
As you could observe before, most of the functions and types are templates that depend on two main arguments: `T` - return type and `Err` - error type that describes execution failure. To reduce the typing effort, one can create template aliases with predefined error types such as `int`, `boost::error_code`, `std::outcome`, etc. The asy::op library provides the aliases for most of the templates with predefined `std::error_code` as the most standard error type in the modern C++. Most of the times, default aliases have the name of the general tempalte without `basic_` prefix: `op_handle<T> -> basic_op_handle<T, std::error_code>`, `asy::context<T> -> asy::basic_context<T, std::error_code>`, `asy::when_all() -> asy::basic_when_all<std::error_code>`, etc.

Please note that default aliases are declared in the optional header, so it is completely fine to replace them with their own custom header. Another way to avoid the collisions is to use another namespace ;)

### Custom callable signature
As was described before, the library supports user-defined continuation types. To make it work, the developer should declare a specialization for `struct asy::continuation<UserType>` with certain methods (the details were described before). If you plan to mix user-defined callables with the continuation specializations from asy::op library, please make sure that user type does not satisfy library-defined concepts: weird compiler errors are expected otherwise.

The user also has a possibility to describe custom callable type in the same manner as asy::op library - using pseudo-concepts or any other method that involves SFINAE technique, the `struct asy::continuation` is SFINAE-ready.

### Custom return type
The paragraph above describes the case when callable type differs from out-of-the-box support provided by asy::op library. The other option is to use a usual functor that returns some user-defined type of object. In that case, the user should create a specialization for `struct asy::simple_continuation<>` from asy/common/simple_continuation.hpp. The same rules apply as before: specialize a template for a concrete type or play with concepts and SFINAE.
<!--stackedit_data:
eyJoaXN0b3J5IjpbMTg0MDk1NzA1Ml19
-->