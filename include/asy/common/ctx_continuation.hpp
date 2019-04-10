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
#include <tuple>
#include "type_traits.hpp"
#include "../core/basic_op_handle.hpp"
#include "../core/basic_context.hpp"


namespace asy::concept
{
    template <typename F>
    using context_arg_first = tt::specialization_of<basic_context, tt::specialization_of_first_t<std::shared_ptr, tt::functor_first_t<F>>>;

    struct CtxContinuation
    {
        template <typename T, typename... Args>
        auto operator()(T&& t, Args&&...)
        -> require<
                is_true<std::is_void_v<tt::functor_ret_t<T>>>,
                is_true<context_arg_first<T>::value>,
                is_true<std::is_invocable_v<T, tt::functor_first_t<T>, Args...>>
        >{}
    };
}

namespace asy
{
    template <typename F, typename... Args>
    struct continuation<F(Args...), c::require<c::satisfy<c::CtxContinuation, F, Args...>>> : std::true_type
    {
        using _shptr = tt::functor_first_t<F>;
        using _ctx = tt::specialization_of_first_t<std::shared_ptr, _shptr>;
        using ret_type = tt::specialization_of_first_t<basic_context, _ctx>;
        using ret_type_orig = void;

        template<typename Err>
        static auto to_handle(std::in_place_type_t<Err>, F&& f, Args&& ... args)
        {
            return asy::basic_op_handle<ret_type, Err>(std::forward<F>(f), std::forward<Args>(args)...);
        }

        template<typename T, typename Err>
        static auto deferred(asy::basic_context_ptr<T, Err> ctx, F&& f)
        {
            return [f = std::forward<F>(f), ctx](Args&& ... args) mutable
            {
                if constexpr (std::is_void_v<ret_type>)
                {
                    to_handle(std::in_place_type<Err>, std::forward<F>(f), std::forward<Args>(args)...).then(
                            [ctx](){ ctx->async_success(); },
                            [ctx](Err&& err){ ctx->async_failure(std::move(err)); });
                }
                else
                {
                    to_handle(std::in_place_type<Err>, std::forward<F>(f), std::forward<Args>(args)...).then(
                            [ctx](ret_type&& output){ ctx->async_success(std::move(output)); },
                            [ctx](Err&& err){ ctx->async_failure(std::move(err)); });
                }
            };
        }
    };
}
