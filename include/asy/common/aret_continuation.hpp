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
#include "type_traits.hpp"
#include "simple_continuation.hpp"

namespace asy::detail::aret_continuation
{
    template <typename F, typename Args>
    constexpr auto check()
    {
        if constexpr (detail::is_appliable_v<F, Args>)
        {
            return detail::specialization_of<asy::basic_op_handle, detail::apply_result_t<F, Args>>::value;
        }
        else
        {
            return false;
        }
    }

    template <typename F, typename Args>
    struct impl: std::conditional_t<check<F, Args>(), std::true_type, std::false_type>{};
}

namespace asy::concept
{
    template <typename F, typename Args>
    inline constexpr auto AretContinuation = detail::aret_continuation::impl<F, Args>::value;

    template <typename F, typename Args>
    using require_AretContinuation = std::enable_if_t<AretContinuation<F, Args>>;
}

namespace asy
{
    template <typename Functor, typename Input>
    struct simple_continuation<Functor, Input, concept::require_AretContinuation<Functor, Input>> : std::true_type
    {
        using ret_type_orig = detail::apply_result_t<Functor, Input>;
        using ret_type = typename detail::specialization_of<asy::basic_op_handle, ret_type_orig>::first_arg;

        template<typename Err, typename F, typename... Args>
        static auto to_handle(F&& f, Args&& ... args)
        {
            return std::forward<F>(f)(std::forward<Args>(args)...);
        }

        template<typename T, typename Err, typename F, typename... Args>
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
