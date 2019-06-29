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

#include "executor.hpp"

#include <functional>
#include <tuple>
#include <variant>
#include <memory>
#include <type_traits>
#include <utility>
#include <mutex>


namespace asy::detail
{
    struct void_t{};
    struct done_t{};

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

    struct context_base
    {
        virtual void cancel() = 0;
        virtual void abort() = 0;
        virtual bool is_done() = 0;
    };
}

namespace asy
{
    /// An operation context that holds current state of the execution and pending continuation or result, if available
    /// \note Not all methods are intended to be called by client code.
    template <typename Val, typename Err>
    class basic_context: public detail::context_base
    {
    public:
        using success_t = typename detail::type_traits<Val>::success;
        using failure_t = Err;
        using success_cb_t = typename detail::type_traits<Val>::success_cb;
        using failure_cb_t = std::function<void(Err&&)>;
        using cb_pair_t = std::tuple<success_cb_t, failure_cb_t>;

        /// Constructor
        basic_context() = default;

        /// Constructor, with parent
        ///
        /// \param parent Pointer to the context of the parent operation
        explicit basic_context(std::shared_ptr<detail::context_base> parent): m_parent(std::move(parent)) {}

        /// Declare a success of the operation
        ///
        /// \param val A value that is interpreted as a result of the operation
        void async_success(success_t&& val = {})
        {
            auto guard = synchronize();
            if (std::get_if<detail::done_t>(&m_pending))
            {
                return;
            }
            else if (auto cbs = std::get_if<cb_pair_t>(&m_pending))
            {
                if constexpr (std::is_void_v<Val>)
                    post(std::get<success_cb_t>(*cbs));
                else
                    post(std::get<success_cb_t>(*cbs), std::move(val));

                m_pending = detail::done_t{};
                m_parent.reset();
            }
            else
            {
                m_pending = std::move(val);
            }
        }

        /// Declare a failure of the operation
        ///
        /// \param val A value that is interpreted as a result of the operation
        void async_failure(failure_t&& val = {})
        {
            auto guard = synchronize();
            if (std::get_if<detail::done_t>(&m_pending))
            {
                return;
            }
            else if (auto cbs = std::get_if<cb_pair_t>(&m_pending))
            {
                post(std::get<failure_cb_t>(*cbs), std::move(val));
                m_pending = detail::done_t{};
                m_parent.reset();
            }
            else
            {
                m_pending = std::move(val);
            }
        }

        /// Declare a completion of the operation. Success overload.
        /// Success/failure is deduced from the argument type
        ///
        /// \param val A value that is interpreted as a result of the operation
        void async_return(success_t&& val = {})
        {
            async_success(std::move(val));
        }

        /// Declare a completion of the operation. Failure overload.
        /// Success/failure is deduced from the argument type
        ///
        /// \param val A value that is interpreted as a result of the operation
        void async_return(failure_t&& val)
        {
            async_failure(std::move(val));
        }

        /// Cancel current operation
        void cancel() override
        {
            auto guard = synchronize();
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
                post(std::get<failure_cb_t>(*cbs), std::move(val));
                m_pending = detail::done_t{};
                m_parent.reset();
            }
            else
            {
                m_pending = std::move(val);
            }
        }

        /// Abort current operation
        void abort() override
        {
            auto guard = synchronize();

            if (m_parent && !m_parent->is_done())
            {
                m_parent->abort();
            }

            m_pending = detail::done_t{};
            m_parent.reset();
        }

        /// Set a pair of callbacks that will be called when result of the operation is ready
        ///
        /// \param success_cb Success callback
        /// \param failure_cb Failure callback
        void set_continuation(success_cb_t success_cb, failure_cb_t failure_cb)
        {
            auto guard = synchronize();
            if (std::get_if<detail::done_t>(&m_pending))
            {
                return;
            }
            else if (auto s_val = std::get_if<success_t>(&m_pending))
            {
                if constexpr (std::is_void_v<Val>)
                    post(success_cb);
                else
                    post(success_cb, std::move(*s_val));
                m_pending = detail::done_t{};
                m_parent.reset();
            }
            else if (auto f_val = std::get_if<failure_t>(&m_pending))
            {
                post(failure_cb, std::move(*f_val));
                m_pending = detail::done_t{};
                m_parent.reset();
            }
            else
            {
                m_pending = cb_pair_t{std::move(success_cb), std::move(failure_cb)};
            }
        }

        /// Check if the current operation is finished
        ///
        /// \return True if operation is finished
        bool is_done() override
        {
            auto guard = synchronize();
            return m_pending.index() == 4;
        }

    private:
        template <typename F, typename... Args>
        void post(F&& f, Args&&... arg)
        {
            if (f)
            {
                executor::get().schedule_execution(
                        [handler = std::forward<F>(f), params = std::make_tuple(std::move(arg)...)]() mutable
                        {
                            std::apply(handler, std::move(params));
                        });
            }
        }

        template <typename F>
        void post(F&& f)
        {
            if (f)
            {
                executor::get().schedule_execution([handler = std::forward<F>(f)]() { handler(); });
            }
        }

        struct sync_guard
        {
            sync_guard(std::mutex& m, bool l) : mutex(m), locked(l) { if (locked) mutex.lock(); }
            ~sync_guard() { if (locked) mutex.unlock(); }

            std::mutex& mutex;
            bool locked;
        };

        sync_guard synchronize()
        {
            return sync_guard(m_mutex, executor::get().should_sync(std::this_thread::get_id()));
        }

        std::variant<std::monostate, cb_pair_t, success_t, failure_t, detail::done_t> m_pending;
        std::shared_ptr<detail::context_base> m_parent;
        std::mutex m_mutex;
    };

    /// Type alias for a context pointer that is used in continuations
    template <typename Ret, typename Err>
    using basic_context_ptr = std::shared_ptr<basic_context<Ret, Err>>;
}
