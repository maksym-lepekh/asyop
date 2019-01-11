// Copyright 2018 Maksym Lepekh
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

#include <functional>
#include <tuple>
#include <variant>
#include <type_traits>
#include <system_error>


namespace asy::detail
{
    struct void_t{};
    struct done_t{};

    template <typename T>
    struct type_traits
    {
        using success = T;
        using success_cb = std::function<void(T&&)>;
        template <typename F> using cb_result = std::invoke_result_t<F, T&&>;
        template <typename F> using std_fun_t = std::function<cb_result<F>(T&&)>;
    };

    template <>
    struct type_traits<void>
    {
        using success = void_t;
        using success_cb = std::function<void()>;
        template <typename F> using cb_result = std::invoke_result_t<F>;
        template <typename F> using std_fun_t = std::function<cb_result<F>()>;
    };

    using posted_fn = std::function<void()>;
    extern thread_local std::function<void(posted_fn)> post_impl;
}

namespace asy
{
    template <typename Val, typename Err>
    class context
    {
    public:
        using success_t = typename detail::type_traits<Val>::success;
        using failure_t = Err;
        using success_cb_t = typename detail::type_traits<Val>::success_cb;
        using failure_cb_t = std::function<void(Err&&)>;
        using cb_pair_t = std::tuple<success_cb_t, failure_cb_t>;

        void async_return(success_t&& val = {})
        {
            if (std::get_if<detail::done_t>(&m_pending))
            {
                return;
            }
            else if (auto cbs = std::get_if<cb_pair_t>(&m_pending))
            {
                if constexpr (std::is_void_v<Val>)
                    post(std::get<success_cb_t>(*cbs));
                else
                    post(std::get<success_cb_t>(*cbs), val);

                m_pending = detail::done_t{};
            }
            else
            {
                m_pending = val;
            }
        }

        void async_return(failure_t&& val)
        {
            if (std::get_if<detail::done_t>(&m_pending))
            {
                return;
            }
            else if (auto cbs = std::get_if<cb_pair_t>(&m_pending))
            {
                post(std::get<failure_cb_t>(*cbs), val);
                m_pending = detail::done_t{};
            }
            else
            {
                m_pending = val;
            }
        }

        void cancel()
        {
            if (std::get_if<detail::done_t>(&m_pending))
            {
                return;
            }

            auto val = std::make_error_code(std::errc::operation_canceled);
            if (auto cbs = std::get_if<cb_pair_t>(&m_pending))
            {
                post(std::get<failure_cb_t>(*cbs), val);
                m_pending = detail::done_t{};
            }
            else
            {
                m_pending = val;
            }
        }

        void set_continuation(success_cb_t success_cb, failure_cb_t failure_cb = {})
        {
            if (std::get_if<detail::done_t>(&m_pending))
            {
                return;
            }
            else if (auto s_val = std::get_if<success_t>(&m_pending))
            {
                if constexpr (std::is_void_v<Val>)
                    post(success_cb);
                else
                    post(success_cb, *s_val);
                m_pending = detail::done_t{};
            }
            else if (auto f_val = std::get_if<failure_t>(&m_pending))
            {
                post(failure_cb, *f_val);
                m_pending = detail::done_t{};
            }
            else
            {
                m_pending = cb_pair_t{std::move(success_cb), std::move(failure_cb)};
            }
        }

    private:
        template <typename F, typename Arg>
        void post(F&& f, Arg&& arg)
        {
            if (f)
            {
                detail::post_impl([handler = std::move(f), param = std::move(arg)]() mutable {
                    handler(std::move(param));
                });
            }
        }

        template <typename F>
        void post(F&& f)
        {
            if (f)
            {
                detail::post_impl([handler = std::move(f)]() {
                    handler();
                });
            }
        }

        std::variant<std::monostate, cb_pair_t, success_t, failure_t, detail::done_t> m_pending;
    };
}
