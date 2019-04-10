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
    struct ValueOrError
    {
        template <typename T> auto impl(T&& t)
        -> require<
                is_true<std::is_convertible_v<decltype(t.has_value()), bool>>,
                is_false<std::is_void_v<decltype(t.value())>>,
                is_false<std::is_void_v<decltype(t.error())>>
        >{}
    };

    struct ValueOrNone
    {
        template <typename T> auto impl(T&& t)
        -> require<
                is_true<std::is_convertible_v<decltype(t.has_value()), bool>>,
                is_false<std::is_void_v<decltype(t.value())>>,
                is_false<satisfies<ValueOrError, T>>
        >{}
    };

    struct NoneOrError
    {
        template <typename T> auto impl(T&& t)
        -> require<
                is_true<std::is_convertible_v<decltype(t.has_value()), bool>>,
                is_false<std::is_void_v<decltype(t.error())>>,
                is_false<satisfies<ValueOrError, T>>
        >{}
    };

    struct VoEContinuation
    {
        template <typename T, typename... Args> auto impl(T&& t, Args&&...)
        -> require<
                is_true<std::is_invocable_v<T, Args...>>,
                satisfy<ValueOrError, std::invoke_result_t<T, Args...>>
        >{}
    };

    struct VoNContinuation
    {
        template <typename T, typename... Args> auto impl(T&& t, Args&&...)
        -> require<
                is_true<std::is_invocable_v<T, Args...>>,
                satisfy<ValueOrNone, std::invoke_result_t<T, Args...>>
        >{}
    };

    struct NoEContinuation
    {
        template <typename T, typename... Args> auto impl(T&& t, Args&&...)
        -> require<
                is_true<std::is_invocable_v<T, Args...>>,
                satisfy<NoneOrError, std::invoke_result_t<T, Args...>>
        >{}
    };
}

namespace asy
{
    template <typename Functor, typename... Input>
    struct simple_continuation<Functor(Input...), c::require<c::satisfy<c::VoEContinuation, Functor, Input...>>>
            : std::true_type
    {
        using ret_type_orig = std::invoke_result_t<Functor, Input...>;
        using ret_type = std::remove_reference_t<decltype(std::declval<ret_type_orig>().value())>;
        using err_type = std::remove_reference_t<decltype(std::declval<ret_type_orig>().error())>;

        template <typename Err, typename F, typename... Args>
        static auto to_handle(F&& f, Args&&... args)
        {
            return basic_op_handle<ret_type, Err>{
                 [](basic_context_ptr<ret_type, Err> ctx, F&& f, Args&&... args)
                 {
                     auto&& ret = f(std::forward<Args>(args)...);
                     if (ret.has_value())
                         ctx->async_success(std::move(ret.value()));
                     else
                         ctx->async_failure(std::move(ret.error()));
                 }, std::forward<F>(f), std::forward<Args>(args)...};
        }

        template <typename T, typename Err, typename F, typename... Args>
        static auto deferred(asy::basic_context_ptr<T, Err> ctx, F&& f)
        {
            return [f = std::forward<F>(f), ctx](Args&&... args)
            {
                auto&& ret = f(std::forward<Args>(args)...);
                if (ret.has_value())
                    ctx->async_success(std::move(ret.value()));
                else
                    ctx->async_failure(std::move(ret.error()));
            };
        }
    };

    template <typename Functor, typename... Input>
    struct simple_continuation<Functor(Input...), c::require<c::satisfy<c::VoNContinuation, Functor, Input...>>>
            : std::true_type
    {
        using ret_type_orig = std::invoke_result_t<Functor, Input...>;
        using ret_type = std::remove_reference_t<decltype(std::declval<ret_type_orig>().value())>;
        using err_type = void;

        template <typename Err, typename F, typename... Args>
        static auto to_handle(F&& f, Args&&... args)
        {
            return basic_op_handle<ret_type, Err>{
                    [](basic_context_ptr<ret_type, Err> ctx, F&& f, Args&&... args)
                    {
                        auto&& ret = f(std::forward<Args>(args)...);
                        if (ret.has_value())
                            ctx->async_success(std::move(ret.value()));
                        else
                            ctx->async_failure();
                    }, std::forward<F>(f), std::forward<Args>(args)...};
        }

        template <typename T, typename Err, typename F, typename... Args>
        static auto deferred(asy::basic_context_ptr<T, Err> ctx, F&& f)
        {
            return [f = std::forward<F>(f), ctx](Args&&... args)
            {
                auto&& ret = f(std::forward<Args>(args)...);
                if (ret.has_value())
                    ctx->async_success(std::move(ret.value()));
                else
                    ctx->async_failure();
            };
        }
    };

    template <typename Functor, typename... Input>
    struct simple_continuation<Functor(Input...), c::require<c::satisfy<c::NoEContinuation, Functor, Input...>>>
            : std::true_type
    {
        using ret_type_orig = std::invoke_result_t<Functor, Input...>;
        using ret_type = void;
        using err_type = std::remove_reference_t<decltype(std::declval<ret_type_orig>().error())>;

        template <typename Err, typename F, typename... Args>
        static auto to_handle(F&& f, Args&&... args)
        {
            return basic_op_handle<ret_type, Err>{
                    [](basic_context_ptr<ret_type, Err> ctx, F&& f, Args&&... args)
                    {
                        auto&& ret = f(std::forward<Args>(args)...);
                        if (ret.has_value())
                            ctx->async_success();
                        else
                            ctx->async_failure(std::move(ret.error()));
                    }, std::forward<F>(f), std::forward<Args>(args)...};
        }

        template <typename T, typename Err, typename F, typename... Args>
        static auto deferred(asy::basic_context_ptr<T, Err> ctx, F&& f)
        {
            return [f = std::forward<F>(f), ctx](Args&&... args)
            {
                auto&& ret = f(std::forward<Args>(args)...);
                if (ret.has_value())
                    ctx->async_success();
                else
                    ctx->async_failure(std::move(ret.error()));
            };
        }
    };
}
