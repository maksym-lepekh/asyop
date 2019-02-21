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
    asio::io_service& get_event_loop();
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

namespace asy { inline namespace asio
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

    template <typename Rep, typename Per>
    auto sleep(std::chrono::duration<Rep, Per> dur)
    {
        auto timer = std::make_shared<::asio::steady_timer>(this_thread::get_event_loop(), dur);
        auto h = fy<>([&](auto&& handler){ timer->async_wait(handler); });

        return add_cancel(h, [timer]{ timer->cancel(); });
    }

    template <typename Rep, typename Per, typename F, typename... Args>
    auto timed_op(std::chrono::duration<Rep, Per> dur, F&& f, Args&&... args)
    {
        auto user_op = asy::op(std::forward<F>(f), std::forward<Args>(args)...);
        auto timer_op = sleep(dur).then([]{ return asy::detail::void_t{}; });

        using ret_t = typename decltype(user_op)::output_t;
        using err_t = typename decltype(user_op)::error_t;
        using input_t = std::variant<ret_t, asy::detail::void_t>;

        return when_any(user_op, timer_op).then([](asy::basic_context_ptr<ret_t, err_t> ctx, input_t&& input)
        {
            if (input.index() == 0)
            {
                if constexpr (std::is_void_v<ret_t>)
                    ctx->async_success();
                else
                    ctx->async_success(std::move(std::get<0>(input)));
            }
            else
            {
                ctx->async_failure(make_error_code(std::errc::timed_out));
            }
        });
    }
}}
