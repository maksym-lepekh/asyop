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

#include "basic_context.hpp"
#include "continuation.hpp"

#include <ostream>
#include <string>
#include <sstream>

namespace asy
{
    /// Client-side handle to the asynchronous operation
    template <typename T, typename Err>
    class basic_op_handle
    {
    public:
        using output_t = T;
        using error_t = Err;

        /// Constructor, no parent
        ///
        /// \param exec A callable that is executed at creation of the operation
        /// \param args Arguments that are forwarder into `exec`
        template <typename Fn, typename... Args>
        explicit basic_op_handle(Fn&& exec, Args&&... args): m_ctx(std::make_shared<basic_context<T, Err>>())
        {
            std::forward<Fn>(exec)(m_ctx, std::forward<Args>(args)...);
        }

        /// Constructor, with parent
        /// Parent-child connection is typically created when `.then()` or similar is used.
        ///
        /// \param parent A pointer to the parent operation context.
        /// \param exec A callable that is executed at creation of the operation
        /// \param args Arguments that are forwarder into `exec`
        template <typename Fn, typename... Args>
        explicit basic_op_handle(std::shared_ptr<detail::context_base> parent, Fn&& exec, Args&&... args)
            : m_ctx(std::make_shared<basic_context<T, Err>>(parent))
        {
            std::forward<Fn>(exec)(m_ctx, std::forward<Args>(args)...);
        }

        /// Cancel the operation
        /// \note Has no effect if operation is already done
        void cancel()
        {
            m_ctx->cancel();
        }

        /// Abort the operation. No continuation must be invoked
        /// \note Has no effect if operation is already done
        void abort()
        {
            m_ctx->abort();
        }

        /// Set the a callable that continues the execution on operation success
        ///
        /// \param fn Continuation, that is compatible with operation return type
        /// \return New handler that corresponds to the continuation
        template <typename Fn>
        auto then(Fn&& fn)
        {
            if constexpr (std::is_void_v<T>)
            {
                using info = continuation<Fn(Err)>;
                using ret_t = typename info::ret_type;

                return basic_op_handle<ret_t, Err>(
                        std::static_pointer_cast<detail::context_base>(m_ctx),
                        [this, &fn](basic_context_ptr<ret_t, Err> ctx)
                        {
                            m_ctx->set_continuation(
                                    info::deferred(ctx, std::forward<Fn>(fn)),
                                    default_skip_failcont<Err>(ctx));
                        });
            }
            else
            {
                using info = continuation<Fn(Err, T&&)>;
                using ret_t = typename info::ret_type;

                return basic_op_handle<ret_t, Err>(
                        std::static_pointer_cast<detail::context_base>(m_ctx),
                        [this, &fn](basic_context_ptr<ret_t, Err> ctx)
                        {
                            m_ctx->set_continuation(
                                    info::deferred(ctx, std::forward<Fn>(fn)),
                                    default_skip_failcont<Err>(ctx));
                        });
            }
        }

        /// Set the a pair of callables that continue the execution on operation success or failure
        ///
        /// \param s Continuation, that is compatible with operation return type
        /// \param f Continuation, that is compatible with operation error type
        /// \return New handler that corresponds to the continuation
        template <typename SuccCb, typename FailCb>
        auto then(SuccCb&& s, FailCb&& f)
        {
            using f_info = continuation<FailCb(Err, Err&&)>;

            if constexpr (std::is_void_v<T>)
            {
                using s_info = continuation<SuccCb(Err)>;
                using ret_t = typename s_info::ret_type;

                return basic_op_handle<ret_t, Err>(
                        std::static_pointer_cast<detail::context_base>(m_ctx),
                        [this, &s, &f](basic_context_ptr<ret_t, Err> ctx)
                        {
                            m_ctx->set_continuation(
                                    s_info::deferred(ctx, std::forward<SuccCb>(s)),
                                    f_info::deferred(ctx, std::forward<FailCb>(f)));
                        });
            }
            else
            {
                using s_info = continuation<SuccCb(Err, T&&)>;
                using ret_t = typename s_info::ret_type;

                return basic_op_handle<ret_t, Err>(
                        std::static_pointer_cast<detail::context_base>(m_ctx),
                        [this, &s, &f](basic_context_ptr<ret_t, Err> ctx)
                        {
                            m_ctx->set_continuation(
                                    s_info::deferred(ctx, std::forward<SuccCb>(s)),
                                    f_info::deferred(ctx, std::forward<FailCb>(f)));
                        });
            }
        }

        /// Set the a callable that continues the execution on operation failure
        ///
        /// \param fn Continuation, that is compatible with operation error type
        /// \return New handler that corresponds to the continuation
        template <typename Fn>
        auto on_failure(Fn&& fn)
        {
            using info = continuation<Fn(Err, Err&&)>;

            return basic_op_handle<void, Err>(
                    std::static_pointer_cast<detail::context_base>(m_ctx),
                    [this, &fn](basic_context_ptr<void, Err> ctx)
                    {
                        m_ctx->set_continuation(
                                default_void_cont<T>(ctx),
                                info::deferred(ctx, std::forward<Fn>(fn)));
                    });
        }

    public:
        friend auto operator<(const basic_op_handle& a, const basic_op_handle& b) noexcept
        {
            return a.m_ctx < b.m_ctx;
        }

        friend auto operator==(const basic_op_handle& a, const basic_op_handle& b) noexcept
        {
            return a.m_ctx == b.m_ctx;
        }

        friend auto operator!=(const basic_op_handle& a, const basic_op_handle& b) noexcept
        {
            return !(a == b);
        }

        friend auto operator>=(const basic_op_handle& a, const basic_op_handle& b) noexcept
        {
            return !(a < b);
        }

        friend auto operator<=(const basic_op_handle& a, const basic_op_handle& b) noexcept
        {
            return (a < b) || (a == b);
        }

        friend auto operator>(const basic_op_handle& a, const basic_op_handle& b) noexcept
        {
            return !(a <= b);
        }

        friend std::hash<basic_op_handle>;

        friend std::ostream& operator<<(std::ostream& os, const basic_op_handle& obj)
        {
            return os << "op_handle{" << obj.m_ctx.get() << "}";
        }

        friend auto to_string(const basic_op_handle& obj)
        {
            auto ss = std::stringstream{};
            ss << obj;
            return ss.str();
        }

    private:
        basic_context_ptr<T, Err> m_ctx;
    };
}

namespace std
{
    template<typename T, typename Err>
    struct hash<asy::basic_op_handle<T, Err>>
    {
        auto operator()(asy::basic_op_handle<T, Err> const& h) const noexcept
        {
            return std::hash<decltype(h.m_ctx)>{}(h.m_ctx);
        }
    };
}
