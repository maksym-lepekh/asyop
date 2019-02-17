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

#include <asy/op.hpp>
#include <asio.hpp>
#include <type_traits>
#include <chrono>

namespace asy::this_thread { inline namespace v1
{
    void set_event_loop(asio::io_service& s);
}}

namespace asy::detail::asio
{
    template <size_t N, typename... Args>
    struct get_ret_impl;

    template <typename... Args>
    struct get_ret_impl<0, Args...> { using type = void; };

    template <typename T>
    struct get_ret_impl<1, T> { using type = T; };

    template <size_t N, typename... Args>
    struct get_ret_impl { using type = std::tuple<Args...>; };

    template <typename... Args>
    using get_ret_t = typename get_ret_impl<sizeof...(Args), Args...>::type;
}

namespace asy::asio
{
    template <typename Call, typename... HandlerArgs>
    auto fy(Call&& call)
    {
        using ret_t = detail::asio::get_ret_t<HandlerArgs...>;
        using err_t = ::asio::error_code;

        return basic_op_handle<ret_t, err_t>(
                [](asy::basic_context_ptr<ret_t, err_t> ctx, Call&& call)
                {
                    std::invoke(call, [ctx](const err_t& e, auto&&... args){
                        if (e)
                        {
                            ctx->async_failure(err_t(e));
                        }
                        else
                        {
                            if constexpr (sizeof...(HandlerArgs) == 0)
                                ctx->async_success();
                            else if constexpr (sizeof...(HandlerArgs) == 1)
                                ctx->async_success(std::forward<HandlerArgs>(args)...);
                            else
                                ctx->async_success(std::forward_as_tuple(args...));
                        }
                    });
                }, std::forward<Call>(call));
    }
}
