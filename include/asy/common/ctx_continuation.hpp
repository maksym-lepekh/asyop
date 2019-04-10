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
    using context_arg_first = detail::specialization_of<basic_context, detail::specialization_of_first_t<std::shared_ptr, detail::functor_first_t<F>>>;

    struct CtxContinuation
    {
        template <typename T, typename Args>
        auto impl(T&& t, Args&&...)
        -> require<
                is_true<std::is_void_v<detail::functor_ret_t<T>>>,
                is_true<context_arg_first<T>::value>,
                is_true<detail::is_appliable_v<T, detail::tuple_prepend_t<typename detail::functor_info<T>::arg1_type, Args>>>
        >{}
    };
}

namespace asy
{
    template <typename Functor, typename Input>
    struct continuation<Functor, Input, c::require<c::satisfy<c::CtxContinuation, Functor, Input>>> : std::true_type
    {
        using _shptr = typename detail::functor_info<Functor>::arg1_type;
        using _ctx = typename detail::specialization_of<std::shared_ptr, _shptr>::first_arg;
        using ret_type = typename detail::specialization_of<basic_context, _ctx>::first_arg;
        using ret_type_orig = void;

        template<typename Err, typename F, typename... Args>
        static auto to_handle(F&& f, Args&& ... args)
        {
            return asy::basic_op_handle<ret_type, Err>(std::forward<F>(f), std::forward<Args>(args)...);
        }

        template<typename T, typename Err, typename F, typename... Args>
        static auto deferred(asy::basic_context_ptr<T, Err> ctx, F&& f)
        {
            return [f = std::forward<F>(f), ctx](Args&& ... args) {
                if constexpr (std::is_void_v<ret_type>)
                {
                    to_handle<Err>(f, std::forward<Args>(args)...).then(
                            [ctx](){ ctx->async_success(); },
                            [ctx](Err&& err){ ctx->async_failure(std::move(err)); });
                }
                else
                {
                    to_handle<Err>(f, std::forward<Args>(args)...).then(
                            [ctx](ret_type&& output){ ctx->async_success(std::move(output)); },
                            [ctx](Err&& err){ ctx->async_failure(std::move(err)); });
                }
            };
        }
    };
}
