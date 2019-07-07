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
    /// Get associated io_service for the current thread
    ///
    /// \return Reference to io_service
    asio::io_service& get_event_loop();

    /// Set a default io_service for the current thread
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

    struct adapt_t{};

    template <typename T, typename Err>
    struct comp_handler_base
    {
        using ret_t = asy::basic_op_handle<T, Err>;

        explicit comp_handler_base(const adapt_t& /*tag*/) {}

        void operator()(asy::basic_context_ptr<T, Err> ctx)
        {
            op_ctx = ctx;
        }

        asy::basic_context_ptr<T, Err> op_ctx;
    };

    template <typename Sign>
    struct comp_handler;

    template <typename Ret, typename Err>
    struct comp_handler<Ret(Err)> : public comp_handler_base<void, Err>
    {
        using base_t = comp_handler_base<void, Err>;
        using base_t::base_t;
        using base_t::operator();

        void operator()(Err ec)
        {
            if (ec)
                base_t::op_ctx->async_failure(std::move(ec));
            else
                base_t::op_ctx->async_success();
        }
    };

    template <typename Ret, typename Err, typename Arg>
    struct comp_handler<Ret(Err, Arg)> : comp_handler_base<Arg, Err>
    {
        using base_t = comp_handler_base<Arg, Err>;
        using base_t::base_t;
        using base_t::operator();

        void operator()(Err ec, Arg arg)
        {
            if (ec)
                base_t::op_ctx->async_failure(std::move(ec));
            else
                base_t::op_ctx->async_success(std::move(arg));
        }
    };

    template <typename Ret, typename Err, typename Arg, typename Arg2, typename... Args>
    struct comp_handler<Ret(Err, Arg, Arg2, Args...)> : comp_handler_base<std::tuple<Arg, Arg2, Args...>, Err>
    {
        using base_t = comp_handler_base<std::tuple<Arg, Arg2, Args...>, Err>;
        using base_t::base_t;
        using base_t::operator();

        void operator()(Err ec, Arg arg, Arg2 arg2, Args... args)
        {
            if (ec)
                base_t::op_ctx->async_failure(std::move(ec));
            else
                base_t::op_ctx->async_success(std::make_tuple(std::move(arg), std::move(arg2), std::move(args)...));
        }
    };
}

namespace asy { inline namespace asio
{
    /// Convert asio async call to asy::op handle using auto-generated completion handler
    ///
    /// This adaptation method requires the client to specify expected handler arguments (without first error code),
    /// then the client must provide a functor which first argument is a proper completion type (it is recommended
    /// to make the operator() a template). Inside the functor, the client should call the asio async method and
    /// forward the completion handler.
    ///
    /// Example:
    /// \code
    /// auto op_handle = asy::asio::fy&lt;std::string&gt;([&](auto completion_handler){
    ///    socket.async_read(std::move(completion_handler));
    /// });
    ///
    /// // decltype(op_handle) -> asy::basic_op_handle&lt;std::string, asio::error_code&gt;
    /// \endcode
    ///
    /// \tparam HandlerArgs List of expected types of the async operation. May be empty
    /// \param call Functor that calls async method
    /// \return Operation handle
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

    /// Sleep using asio::steady_timer and current thread's asio::io_service
    ///
    /// \param dur Duration of sleep
    /// \return Operation handle
    template <typename Rep, typename Per>
    auto sleep(std::chrono::duration<Rep, Per> dur)
    {
        auto timer = std::make_shared<::asio::steady_timer>(this_thread::get_event_loop(), dur);
        auto h = fy<>([&](auto&& handler){ timer->async_wait(handler); });

        return add_cancel(h, [timer]{ timer->cancel(); });
    }

    /// Start asynchronous operation with timeout
    ///
    /// This compound operation describes a race (same as when_any()) between user specified operation and
    /// a timer of specified duration. The first finished operation cancels the other one.
    ///
    /// \param dur Duration until timeout
    /// \param f Functor that describes an operation (it will be converted to an operation handle using asy::op())
    /// \param args Optional arguments for the functor
    /// \return Operation handle
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

    /// Special tag that is used to convert asio asynchronous operation to asy::op handle using
    /// asio's async_result&lt;Signature&gt; mechanism
    ///
    /// When passed instead of completion handle, the asio's async method will return asy::op_handle
    constexpr detail::asio::adapt_t adapt;
}}

namespace asio
{
    template<typename Signature>
    struct async_result<asy::detail::asio::adapt_t, Signature>
    {
        using completion_handler_type = asy::detail::asio::comp_handler<Signature>;
        using return_type = typename completion_handler_type::ret_t;

        explicit async_result(completion_handler_type& h) : m_handle(h) { }
        return_type get() { return m_handle; }

    private:
        return_type m_handle;
    };
}
