// Copyright 2018-2019 Maksym Lepekh
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#pragma once

#include <type_traits>
#include <asy/core/basic_op_handle.hpp>
#include <asy/core/basic_context.hpp>
#include <asy/core/support/concept.hpp>
#include "simple_continuation.hpp"
#include "type_traits.hpp"


namespace asy::concept
{
    /// "Value or error" concept
    struct ValueOrError
    {
        template <typename T> auto operator()(T&& t)
        -> require<
                is_true<std::is_convertible_v<decltype(t.has_value()), bool>>,
                is_false<std::is_void_v<decltype(t.value())>>,
                is_false<std::is_void_v<decltype(t.error())>>
        >{}
    };

    /// "Value or none" concept
    struct ValueOrNone
    {
        template <typename T> auto operator()(T&& t)
        -> require<
                is_true<std::is_convertible_v<decltype(t.has_value()), bool>>,
                is_false<std::is_void_v<decltype(t.value())>>,
                is_false<satisfies<ValueOrError, T>>
        >{}
    };

    /// "None or error" concept
    struct NoneOrError
    {
        template <typename T> auto operator()(T&& t)
        -> require<
                is_true<std::is_convertible_v<decltype(t.has_value()), bool>>,
                is_false<std::is_void_v<decltype(t.error())>>,
                is_false<satisfies<ValueOrError, T>>
        >{}
    };

    /// "Value or error" continuation concept
    struct VoEContinuation
    {
        template <typename T, typename... Args> auto operator()(T&& /*t*/, Args&&... /*args*/)
        -> require<
                is_true<std::is_invocable_v<T, Args...>>,
                satisfy<ValueOrError, std::invoke_result_t<T, Args...>>
        >{}
    };

    /// "Value or none" continuation concept
    struct VoNContinuation
    {
        template <typename T, typename... Args> auto operator()(T&& /*t*/, Args&&... /*args*/)
        -> require<
                is_true<std::is_invocable_v<T, Args...>>,
                satisfy<ValueOrNone, std::invoke_result_t<T, Args...>>
        >{}
    };

    /// "None or error" continuation concept
    struct NoEContinuation
    {
        template <typename T, typename... Args> auto operator()(T&& /*t*/, Args&&... /*args*/)
        -> require<
                is_true<std::is_invocable_v<T, Args...>>,
                satisfy<NoneOrError, std::invoke_result_t<T, Args...>>
        >{}
    };
}

namespace asy
{
    /// Default support for ValueOrError continuation.
    /// \see struct asy::continuation
    template <typename F, typename... Args>
    struct simple_continuation<F(Args...), std::enable_if_t<c::satisfies<c::VoEContinuation, F, Args...>>>
            : std::true_type
    {
        using ret_type_orig = std::invoke_result_t<F, Args...>;
        using ret_type = std::remove_reference_t<decltype(std::declval<ret_type_orig>().value())>;
        using err_type = std::remove_reference_t<decltype(std::declval<ret_type_orig>().error())>;

        template <typename Err>
        static auto to_handle(std::in_place_type_t<Err> /*err type*/, F&& f, Args&&... args)
        {
            return basic_op_handle<ret_type, Err>{
                 [](basic_context_ptr<ret_type, Err> ctx, F&& f, Args&&... args)
                 {
                     auto&& ret = f(std::forward<Args>(args)...);
                     if (ret.has_value())
                     {
                         ctx->async_success(std::move(ret.value()));
                     }
                     else
                     {
                         ctx->async_failure(std::move(ret.error()));
                     }
                 }, std::forward<F>(f), std::forward<Args>(args)...};
        }

        template <typename T, typename Err>
        static auto deferred(asy::basic_context_ptr<T, Err> ctx, F&& f)
        {
            return [f = std::forward<F>(f), ctx](Args&&... args)
            {
                auto&& ret = f(std::forward<Args>(args)...);
                if (ret.has_value())
                {
                    ctx->async_success(std::move(ret.value()));
                }
                else
                {
                    ctx->async_failure(std::move(ret.error()));
                }
            };
        }
    };

    /// Default support for ValueOrNone continuation.
    /// \see struct asy::continuation
    template <typename F, typename... Args>
    struct simple_continuation<F(Args...), std::enable_if_t<c::satisfies<c::VoNContinuation, F, Args...>>>
            : std::true_type
    {
        using ret_type_orig = std::invoke_result_t<F, Args...>;
        using ret_type = std::remove_reference_t<decltype(std::declval<ret_type_orig>().value())>;
        using err_type = void;

        template <typename Err>
        static auto to_handle(std::in_place_type_t<Err> /*err type*/, F&& f, Args&&... args)
        {
            return basic_op_handle<ret_type, Err>{
                    [](basic_context_ptr<ret_type, Err> ctx, F&& f, Args&&... args)
                    {
                        auto&& ret = f(std::forward<Args>(args)...);
                        if (ret.has_value())
                        {
                            ctx->async_success(std::move(ret.value()));
                        }
                        else
                        {
                            ctx->async_failure();
                        }
                    }, std::forward<F>(f), std::forward<Args>(args)...};
        }

        template <typename T, typename Err>
        static auto deferred(asy::basic_context_ptr<T, Err> ctx, F&& f)
        {
            return [f = std::forward<F>(f), ctx](Args&&... args)
            {
                auto&& ret = f(std::forward<Args>(args)...);
                if (ret.has_value())
                {
                    ctx->async_success(std::move(ret.value()));
                }
                else
                {
                    ctx->async_failure();
                }
            };
        }
    };

    /// Default support for NoneOrError continuation.
    /// \see struct asy::continuation
    template <typename F, typename... Args>
    struct simple_continuation<F(Args...), std::enable_if_t<c::satisfies<c::NoEContinuation, F, Args...>>>
            : std::true_type
    {
        using ret_type_orig = std::invoke_result_t<F, Args...>;
        using ret_type = void;
        using err_type = std::remove_reference_t<decltype(std::declval<ret_type_orig>().error())>;

        template <typename Err>
        static auto to_handle(std::in_place_type_t<Err> /*err type*/, F&& f, Args&&... args)
        {
            return basic_op_handle<ret_type, Err>{
                    [](basic_context_ptr<ret_type, Err> ctx, F&& f, Args&&... args)
                    {
                        auto&& ret = f(std::forward<Args>(args)...);
                        if (ret.has_value())
                        {
                            ctx->async_success();
                        }
                        else
                        {
                            ctx->async_failure(std::move(ret.error()));
                        }
                    }, std::forward<F>(f), std::forward<Args>(args)...};
        }

        template <typename T, typename Err>
        static auto deferred(asy::basic_context_ptr<T, Err> ctx, F&& f)
        {
            return [f = std::forward<F>(f), ctx](Args&&... args)
            {
                auto&& ret = f(std::forward<Args>(args)...);
                if (ret.has_value())
                {
                    ctx->async_success();
                }
                else
                {
                    ctx->async_failure(std::move(ret.error()));
                }
            };
        }
    };
}
