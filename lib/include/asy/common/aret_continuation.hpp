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
#include "type_traits.hpp"
#include "simple_continuation.hpp"


namespace asy::concept
{
    /// "Async return" continuation concept
    struct ARetContinuation
    {
        template <typename T, typename... Args> auto operator()(T&& /*t*/, Args&&... /*args*/)
        -> require<
                is_true<std::is_invocable_v<T, Args...>>,
                is_true<tt::specialization_of<asy::basic_op_handle, std::invoke_result_t<T, Args...>>::value>
        >{}
    };
}

namespace asy
{
    /// Default support for "async return" continuation.
    /// \see struct asy::continuation
    template <typename F, typename... Args>
    struct simple_continuation<F(Args...), c::require<c::satisfy<c::ARetContinuation, F, Args...>>>
            : std::true_type
    {
        using ret_type_orig = std::invoke_result_t<F, Args...>;
        using ret_type = typename tt::specialization_of<asy::basic_op_handle, ret_type_orig>::first_arg;

        template<typename Err>
        static auto to_handle(std::in_place_type_t<Err> /*err type*/, F&& f, Args&& ... args)
        {
            return std::forward<F>(f)(std::forward<Args>(args)...);
        }

        template<typename T, typename Err>
        static auto deferred(asy::basic_context_ptr<T, Err> ctx, F&& f)
        {
            return [f = std::forward<F>(f), ctx](Args&& ... args) {
                auto&& handle = f(std::forward<Args>(args)...);
                if constexpr (std::is_void_v<ret_type>)
                {
                    handle.then([ctx]() { ctx->async_success(); },
                                [ctx](auto&& err) { ctx->async_failure(std::forward<decltype(err)>(err)); });
                }
                else
                {
                    handle.then([ctx](ret_type&& output) { ctx->async_success(std::move(output)); },
                                [ctx](auto&& err) { ctx->async_failure(std::forward<decltype(err)>(err)); });
                }
            };
        }
    };
}
