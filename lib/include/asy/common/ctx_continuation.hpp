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
#include "util.hpp"
#include "../core/basic_op_handle.hpp"
#include "../core/basic_context.hpp"


namespace asy::concept
{
    template <typename F>
    using context_arg_first = util::specialization_of<basic_context, util::specialization_of_first_t<std::shared_ptr, util::functor_first_t<F>>>;

    template <typename F>
    using context_get_second_arg = util::specialization_of_second_t<basic_context, util::specialization_of_first_t<std::shared_ptr, util::functor_first_t<F>>>;

    /// Concept of the continuation with a context argument
    struct CtxContinuation
    {
        template <typename T, typename Err, typename... Args>
        auto operator()(T&& /*t*/, Err&& /*err*/, Args&&... /*args*/)
        -> require<
                is_true<std::is_void_v<util::functor_ret_t<T>>>,
                is_true<context_arg_first<T>::value>,
                is_true<std::is_invocable_v<T, util::functor_first_t<T>, Args...>>,
                is_true<std::is_convertible_v<context_get_second_arg<T>, Err>>
        >{}
    };
}

namespace asy
{
    /// Default support for continuation with a context argument.
    /// \see struct asy::continuation
    template <typename F, typename Err, typename... Args>
    struct continuation<F(Err, Args...), std::enable_if_t<c::satisfies<c::CtxContinuation, F, Err, Args...>>> : std::true_type
    {
        using _shptr = util::functor_first_t<F>;
        using _ctx = util::specialization_of_first_t<std::shared_ptr, _shptr>;
        using ret_type = util::specialization_of_first_t<basic_context, _ctx>;
        using ret_type_orig = void;

        static auto to_handle(F&& f, Args&& ... args)
        {
            if constexpr (util::should_catch<Err, F, Args...>)
            {
                ASYOP_TRY
                {
                    return asy::basic_op_handle<ret_type, Err>(std::forward<F>(f), std::forward<Args>(args)...);
                }
                ASYOP_CATCH
                {
                    return asy::basic_op_handle<ret_type, Err>([e = std::current_exception()](auto ctx)
                    {
                        ctx->async_failure(e);
                    });
                }
            }
            else
            {
                return asy::basic_op_handle<ret_type, Err>(std::forward<F>(f), std::forward<Args>(args)...);
            }
        }

        template<typename T, typename E>
        static auto deferred(asy::basic_context_ptr<T, E> ctx, F&& f)
        {
            return [f = std::forward<F>(f), ctx](Args&& ... args) mutable
            {
                util::safe_invoke(ctx, []{}, f, ctx, std::forward<Args>(args)...);
            };
        }
    };
}
