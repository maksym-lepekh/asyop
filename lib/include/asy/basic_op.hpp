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

#include "core/basic_context.hpp"
#include "core/basic_op_handle.hpp"
#include "core/continuation.hpp"
#include "common/type_traits.hpp"

namespace asy
{
    /// Create and start an operation
    ///
    /// \tparam Err Error type of the operation
    /// \param fn Functor that represents a computation or result of the finished operation or operation handle
    /// \param args Functor arguments
    /// \return Operation handle
    template <typename Err, typename F, typename... Args>
    auto basic_op(F&& fn, Args&&... args)
    {
        if constexpr (tt::specialization_of<basic_op_handle, std::decay_t<F>>::value && sizeof...(Args) == 0)
        {
            return std::forward<F>(fn);
        }
        else if constexpr (asy::continuation<F(Args...)>::value)
        {
            return asy::continuation<F(Args...)>::to_handle(std::in_place_type<Err>, std::forward<F>(fn), std::forward<Args>(args)...);
        }
        else if constexpr (sizeof...(Args) == 0)
        {
            using ret_t = std::decay_t<F>;
            return basic_op_handle<ret_t, Err>{[](basic_context_ptr<ret_t, Err> ctx, F&& init){
                ctx->async_return(std::forward<F>(init));
            }, std::forward<F>(fn)};
        }
        else
        {
            static_assert(sizeof...(Args) == 0, "Invalid argument type");
        }
    }

    /// Create a finished operation with a pending default-constructed result
    ///
    /// \tparam Ret Error type of the operation
    /// \tparam Err Return type of the operation
    /// \return Operation handle
    template <typename Ret, typename Err>
    auto basic_op()
    {
        return basic_op_handle<Ret, Err>{[](basic_context_ptr<Ret, Err> ctx){
            ctx->async_return();
        }};
    }
}
