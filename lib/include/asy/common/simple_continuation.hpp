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
#include <asy/core/basic_context.hpp>
#include <asy/core/basic_op_handle.hpp>
#include <asy/core/support/concept.hpp>
#include "type_traits.hpp"


namespace asy::concept
{
    /// "Simple" continuation concept
    struct SimpleContinuation
    {
        template <typename T, typename... Args>
        auto operator()(T&& /*t*/, Args&&... /*args*/)
        -> require<
                is_true<std::is_invocable_v<T, Args...>>
        >{}
    };
}

namespace asy
{
    template <typename F>
    struct simple_continuation_impl;

    template <typename F, typename... Args>
    struct simple_continuation_impl<F(Args...)> : std::true_type
    {
        using ret_type = std::invoke_result_t<F, Args...>;

        template <typename Err>
        static auto to_handle(std::in_place_type_t<Err> /*err type*/, F&& f, Args&&... args)
        {
            return basic_op_handle<ret_type, Err>{
                [](basic_context_ptr<ret_type, Err> ctx, F&& f, Args&&... args)
                {
                    invoke(ctx, std::forward<F>(f), std::forward<Args>(args)...);
                }, std::forward<F>(f), std::forward<Args>(args)...};
        }

        template <typename T, typename Err>
        static auto deferred(asy::basic_context_ptr<T, Err> ctx, F&& f)
        {
            return [f = std::forward<F>(f), ctx](Args&&... args) mutable
            {
                invoke(ctx, std::forward<F>(f), std::forward<Args>(args)...);
            };
        }

    private:
        template <typename T, typename Err>
        static auto invoke(asy::basic_context_ptr<T, Err> ctx, F&& f, Args&&... args)
        {
            if constexpr (std::is_void_v<ret_type>)
            {
                f(std::forward<Args>(args)...);
                ctx->async_success();
            }
            else
            {
                ctx->async_success(f(std::forward<Args>(args)...));
            }
        }
    };

    template <typename Proto, typename Sfinae = void>
    struct simple_continuation : simple_continuation_impl<Proto>{};

    /// Default support for simple continuation.
    /// \see struct asy::continuation
    template <typename Functor, typename... Input>
    struct continuation<Functor(Input...), std::enable_if_t<c::satisfies<c::SimpleContinuation, Functor, Input...>>>
            : simple_continuation<Functor(Input...)>{};
}
