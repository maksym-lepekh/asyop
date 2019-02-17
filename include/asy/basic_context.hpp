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

#include <functional>
#include <tuple>
#include <variant>
#include <memory>
#include <type_traits>
#include <utility>


namespace asy::detail
{
    struct void_t
    {
    };
    struct done_t
    {
    };

    template<typename T>
    struct type_traits
    {
        using success = T;
        using success_cb = std::function<void(T&&)>;
        template<typename F> using cb_result = std::invoke_result_t<F, T&&>;
        template<typename F> using std_fun_t = std::function<cb_result<F>(T&&)>;
    };

    template<>
    struct type_traits<void>
    {
        using success = void_t;
        using success_cb = std::function<void()>;
        template<typename F> using cb_result = std::invoke_result_t<F>;
        template<typename F> using std_fun_t = std::function<cb_result<F>()>;
    };

    template<typename Err>
    struct error_traits;

    inline namespace v1
    {
        using posted_fn = std::function<void()>;
        extern thread_local std::function<void(posted_fn)> post_impl;
    }

    struct context_base
    {
        virtual void cancel() = 0;
        virtual bool is_done() = 0;
    };
}

namespace asy
{
    template <typename Val, typename Err>
    class basic_context: public detail::context_base
    {
    public:
        using success_t = typename detail::type_traits<Val>::success;
        using failure_t = Err;
        using success_cb_t = typename detail::type_traits<Val>::success_cb;
        using failure_cb_t = std::function<void(Err&&)>;
        using cb_pair_t = std::tuple<success_cb_t, failure_cb_t>;

        basic_context() = default;

        explicit basic_context(std::shared_ptr<detail::context_base> parent): m_parent(std::move(parent)) {}

        void async_success(success_t&& val = {})
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
                m_parent.reset();
            }
            else
            {
                m_pending = val;
            }
        }

        void async_failure(failure_t&& val = {})
        {
            if (std::get_if<detail::done_t>(&m_pending))
            {
                return;
            }
            else if (auto cbs = std::get_if<cb_pair_t>(&m_pending))
            {
                post(std::get<failure_cb_t>(*cbs), val);
                m_pending = detail::done_t{};
                m_parent.reset();
            }
            else
            {
                m_pending = val;
            }
        }

        void async_return(success_t&& val = {})
        {
            async_success(std::move(val));
        }

        void async_return(failure_t&& val)
        {
            async_failure(std::move(val));
        }

        void cancel() override
        {
            if (std::get_if<detail::done_t>(&m_pending))
            {
                return;
            }

            if (m_parent && !m_parent->is_done())
            {
                m_parent->cancel();
                return;
            }

            auto val = detail::error_traits<Err>::get_cancelled();
            if (auto cbs = std::get_if<cb_pair_t>(&m_pending))
            {
                post(std::get<failure_cb_t>(*cbs), val);
                m_pending = detail::done_t{};
                m_parent.reset();
            }
            else
            {
                m_pending = val;
            }
        }

        void set_continuation(success_cb_t success_cb, failure_cb_t failure_cb)
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
                m_parent.reset();
            }
            else if (auto f_val = std::get_if<failure_t>(&m_pending))
            {
                post(failure_cb, *f_val);
                m_pending = detail::done_t{};
                m_parent.reset();
            }
            else
            {
                m_pending = cb_pair_t{std::move(success_cb), std::move(failure_cb)};
            }
        }

        bool is_done() override
        {
            return m_pending.index() == 4;
        }

    private:
        template <typename F, typename Arg>
        void post(F&& f, Arg&& arg)
        {
            if (f)
            {
                detail::post_impl([handler = std::forward<F>(f), param = std::forward<Arg>(arg)]() mutable {
                    handler(std::move(param));
                });
            }
        }

        template <typename F>
        void post(F&& f)
        {
            if (f)
            {
                detail::post_impl([handler = std::forward<F>(f)]() {
                    handler();
                });
            }
        }

        std::variant<std::monostate, cb_pair_t, success_t, failure_t, detail::done_t> m_pending;
        std::shared_ptr<detail::context_base> m_parent;
    };

    template <typename Ret, typename Err>
    using basic_context_ptr = std::shared_ptr<basic_context<Ret, Err>>;
}
