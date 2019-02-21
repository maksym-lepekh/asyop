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

#include "detail/continuation_info.hpp"
#include "basic_context.hpp"
#include "basic_op_handle.hpp"

namespace asy
{
    template <typename Err, typename F, typename... Args>
    auto basic_op(F&& fn, Args&&... args)
    {
        using info = detail::continuation_info<F, void, Args...>;

        if constexpr (detail::specialization_of<basic_op_handle, std::decay_t<F>>::value && sizeof...(Args) == 0)
        {
            return std::forward<F>(fn);
        }
        else if constexpr (info::type == detail::cont_type::invalid && sizeof...(Args) == 0)
        {
            using ret_t = std::decay_t<F>;
            return basic_op_handle<ret_t, Err>{[](basic_context_ptr<ret_t, Err> ctx, F&& init){
                ctx->async_return(std::forward<F>(init));
            }, std::forward<F>(fn)};
        }
        else
        {
            static_assert(info::type != detail::cont_type::invalid, "Functor has unsupported type");
            using ret_t = typename info::ret_type;

            if constexpr (info::type == detail::cont_type::simple || info::type == detail::cont_type::ambiguous_simple)
            {
                return basic_op_handle<ret_t, Err>{[&fn](basic_context_ptr<ret_t, Err> ctx, Args&&... args){
                    ctx->async_return(fn(std::forward<Args>(args)...));
                }, std::forward<Args>(args)...};
            }
            else if constexpr (info::type == detail::cont_type::areturn || info::type == detail::cont_type::ambiguous_areturn)
            {
                return fn(std::forward<Args>(args)...);
            }
            else
            {
                return basic_op_handle<ret_t, Err>{fn, std::forward<Args>(args)...};
            }
        }
    }



    template <typename Ret, typename Err>
    auto basic_op()
    {
        return basic_op_handle<Ret, Err>{[](basic_context_ptr<Ret, Err> ctx){
            ctx->async_return();
        }};
    }
}
