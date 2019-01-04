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
#include <system_error>

#include <iostream>


namespace asy
{
    namespace detail
    {
        using posted_fn = std::function<void()>;
        extern thread_local std::function<void(posted_fn)> post_impl;
    }

    class basic_context
    {
    protected:
        struct done_tag {};

        template <typename F, typename... Arg>
        void post(F& f, Arg&... arg)
        {
            if (f)
            {
                detail::post_impl([handler = std::move(f), params = std::make_tuple(std::move(arg)...)]() mutable {
                    std::apply(handler, params);
                });
            }
        }
    };


    template <typename T>
    class context: public basic_context
    {
    public:
        using success_t = T;
        using failure_t = std::error_code;

        using success_cb_t = std::function<void(success_t&)>;
        using failure_cb_t = std::function<void(failure_t&)>;

        void async_return(success_t&& val)
        {
            if (std::get_if<done_tag>(&m_pending))
            {
                return;
            }
            else if (auto cbs = std::get_if<cb_pair_t>(&m_pending))
            {
                post(std::get<success_cb_t>(*cbs), val);
                m_pending = done_tag{};
            }
            else
            {
                m_pending = val;
            }
        }

        void async_return(failure_t&& val)
        {
            if (std::get_if<done_tag>(&m_pending))
            {
                return;
            }
            else if (auto cbs = std::get_if<cb_pair_t>(&m_pending))
            {
                post(std::get<failure_cb_t>(*cbs), val);
                m_pending = done_tag{};
            }
            else
            {
                m_pending = val;
            }
        }

        void cancel()
        {
            if (std::get_if<done_tag>(&m_pending))
            {
                return;
            }

            auto val = std::make_error_code(std::errc::operation_canceled);
            if (auto cbs = std::get_if<cb_pair_t>(&m_pending))
            {
                post(std::get<failure_cb_t>(*cbs), val);
                m_pending = done_tag{};
            }
            else
            {
                m_pending = val;
            }
        }

        void then(success_cb_t success_cb, failure_cb_t failure_cb = {})
        {
            if (std::get_if<done_tag>(&m_pending))
            {
                return;
            }
            else if (auto s_val = std::get_if<success_t>(&m_pending))
            {
                post(success_cb, *s_val);
                m_pending = done_tag{};
            }
            else if (auto f_val = std::get_if<failure_t>(&m_pending))
            {
                post(failure_cb, *f_val);
                m_pending = done_tag{};
            }
            else
            {
                m_pending = cb_pair_t{std::move(success_cb), std::move(failure_cb)};
            }
        }

        ~context()
        {
            std::cout << "context dtor, state: " << m_pending.index() << "\n";
        }

    private:
        using cb_pair_t = std::tuple<success_cb_t, failure_cb_t>;
        std::variant<std::monostate, cb_pair_t, success_t, failure_t, done_tag> m_pending;
    };

    template <>
    class context<void>: public basic_context
    {
    public:
        struct success_t{};
        using failure_t = std::error_code;

        using success_cb_t = std::function<void()>;
        using failure_cb_t = std::function<void(failure_t&)>;

        void async_return()
        {
            if (std::get_if<done_tag>(&m_pending))
            {
                return;
            }
            else if (auto cbs = std::get_if<cb_pair_t>(&m_pending))
            {
                post(std::get<success_cb_t>(*cbs));
                m_pending = done_tag{};
            }
            else
            {
                m_pending = success_t{};
            }
        }

        void async_return(failure_t&& val)
        {
            if (std::get_if<done_tag>(&m_pending))
            {
                return;
            }
            else if (auto cbs = std::get_if<cb_pair_t>(&m_pending))
            {
                post(std::get<failure_cb_t>(*cbs), val);
                m_pending = done_tag{};
            }
            else
            {
                m_pending = val;
            }
        }

        void cancel()
        {
            if (std::get_if<done_tag>(&m_pending))
            {
                return;
            }

            auto val = std::make_error_code(std::errc::operation_canceled);
            if (auto cbs = std::get_if<cb_pair_t>(&m_pending))
            {
                post(std::get<failure_cb_t>(*cbs), val);
                m_pending = done_tag{};
            }
            else
            {
                m_pending = val;
            }
        }

        void then(success_cb_t success_cb, failure_cb_t failure_cb = {})
        {
            if (std::get_if<done_tag>(&m_pending))
            {
                return;
            }
            else if (std::get_if<success_t>(&m_pending))
            {
                post(success_cb);
                m_pending = done_tag{};
            }
            else if (auto f_val = std::get_if<failure_t>(&m_pending))
            {
                post(failure_cb, *f_val);
                m_pending = done_tag{};
            }
            else
            {
                m_pending = cb_pair_t{std::move(success_cb), std::move(failure_cb)};
            }
        }

        ~context()
        {
            std::cout << "context dtor, state: " << m_pending.index() << "\n";
        }

    private:
        using cb_pair_t = std::tuple<success_cb_t, failure_cb_t>;
        std::variant<std::monostate, cb_pair_t, success_t, failure_t, done_tag> m_pending;
    };
}
