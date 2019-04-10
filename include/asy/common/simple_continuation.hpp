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
#include "type_traits.hpp"


namespace asy::concept
{
    struct SimpleContinuation
    {
        template <typename T, typename Args> auto impl(T&& t, Args&&...)
        -> require<
                is_true<detail::is_appliable_v<T, Args>>
        >{}
    };
}

namespace asy
{
    template <typename Functor, typename Input, typename Sfinae = void>
    struct simple_continuation : std::true_type
    {
        using ret_type = detail::apply_result_t<Functor, Input>;

        template <typename Err, typename F, typename... Args>
        static auto to_handle(F&& f, Args&&... args)
        {
            return basic_op_handle<ret_type, Err>{
                [](basic_context_ptr<ret_type, Err> ctx, F&& f, Args&&... args)
                {
                    invoke(ctx, f, std::forward<Args>(args)...);
                }, std::forward<F>(f), std::forward<Args>(args)...};
        }

        template <typename T, typename Err, typename F, typename... Args>
        static auto deferred(asy::basic_context_ptr<T, Err> ctx, F&& f)
        {
            return [f = std::forward<F>(f), ctx](Args&&... args)
            {
                invoke(ctx, f, std::forward<Args>(args)...);
            };
        }

    private:
        template <typename T, typename Err, typename F, typename... Args>
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

    template <typename Functor, typename Input>
    struct continuation<Functor, Input, c::require<c::satisfy<c::SimpleContinuation, Functor, Input>>>
            : simple_continuation<Functor, Input>{};
}
