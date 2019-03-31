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

#include <asy/core/basic_op_handle.hpp>
#include <asy/core/basic_context.hpp>
#include "value_or_error.hpp"
#include "simple_continuation.hpp"

namespace asy::detail::voe_continuation
{
    template <typename F, typename Args>
    constexpr auto check()
    {
        if constexpr (detail::is_appliable_v<F, Args>)
        {
            return detail::is_ValueOrError<detail::apply_result_t<F, Args>>;
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
    inline constexpr auto VoEContinuation = detail::voe_continuation::impl<F, Args>::value;

    template <typename F, typename Args>
    using require_VoEContinuation = std::enable_if_t<VoEContinuation<F, Args>>;
}

namespace asy
{
    template <typename Functor, typename Input>
    struct simple_continuation<Functor, Input, concept::require_VoEContinuation<Functor, Input>> : std::true_type
    {
        using ret_type_orig = detail::apply_result_t<Functor, Input>;
        using ret_type = typename detail::ValueOrError<ret_type_orig>::success_type;
        static constexpr auto voe_type = detail::ValueOrError<ret_type_orig>::voe_type;

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
            return [f = std::forward<F>(f), ctx](Args&&... input)
            {
                invoke(ctx, f, std::forward<Args>(input)...);
            };
        }

    private:
        template <typename T, typename Err, typename F, typename... Args>
        static auto invoke(asy::basic_context_ptr<T, Err> ctx, F&& f, Args&&... args)
        {
            auto&& ret = f(std::forward<Args>(args)...);

            if constexpr (voe_type == detail::voe_t::voe) {
                if (ret.has_value())
                    ctx->async_success(std::move(ret.value()));
                else
                    ctx->async_failure(std::move(ret.error()));
            } else if constexpr (voe_type == detail::voe_t::von) {
                if (ret.has_value())
                    ctx->async_success(std::move(ret.value()));
                else
                    ctx->async_failure();
            } else if constexpr (voe_type == detail::voe_t::noe) {
                if (ret.has_value())
                    ctx->async_success();
                else
                    ctx->async_failure(std::move(ret.error()));
            }
        }
    };
}
