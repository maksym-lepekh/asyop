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
#include "../core/basic_context.hpp"
#include "../core/basic_op_handle.hpp"
#include "../core/support/concept.hpp"
#include "util.hpp"


namespace asy::concept
{
    /// "Simple" continuation concept
    struct SimpleContinuation
    {
        template <typename T, typename Err, typename... Args>
        auto operator()(T&& /*t*/, Err&& /*err*/, Args&&... /*args*/)
        -> require<
                is_true<std::is_invocable_v<T, Args...>>
        >{}
    };
}

namespace asy
{
    template <typename F>
    struct simple_continuation_impl;

    template <typename F, typename Err, typename... Args>
    struct simple_continuation_impl<F(Err, Args...)> : std::true_type
    {
        using ret_type = std::invoke_result_t<F, Args...>;

        static auto to_handle(F&& f, Args&&... args)
        {
            return basic_op_handle<ret_type, Err>{
                [](basic_context_ptr<ret_type, Err> ctx, F&& f, Args&&... args)
                {
                    invoke(ctx, std::forward<F>(f), std::forward<Args>(args)...);
                }, std::forward<F>(f), std::forward<Args>(args)...};
        }

        template <typename T, typename E>
        static auto deferred(asy::basic_context_ptr<T, E> ctx, F&& f)
        {
            return [f = std::forward<F>(f), ctx](Args&&... args) mutable
            {
                invoke(ctx, std::forward<F>(f), std::forward<Args>(args)...);
            };
        }

    private:
        template <typename T, typename E>
        static auto invoke(asy::basic_context_ptr<T, E> ctx, F&& f, Args&&... args)
        {
            util::safe_invoke(ctx, [&ctx](auto&&... ret)
            {
                ctx->async_success(std::forward<decltype(ret)>(ret)...);
            }, std::forward<F>(f), std::forward<Args>(args)...);
        }
    };

    template <typename Proto, typename Sfinae = void>
    struct simple_continuation : simple_continuation_impl<Proto>{};

    /// Default support for simple continuation.
    /// \see struct asy::continuation
    template <typename Functor, typename Error, typename... Input>
    struct continuation<Functor(Error, Input...), std::enable_if_t<c::satisfies<c::SimpleContinuation, Functor, Error, Input...>>>
            : simple_continuation<Functor(Error, Input...)>{};
}
