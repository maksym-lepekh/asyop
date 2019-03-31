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

namespace asy
{
    template <typename T, typename Err>
    class basic_op_handle
    {
    public:
        using output_t = T;
        using error_t = Err;

        template <typename Fn, typename... Args>
        explicit basic_op_handle(Fn&& exec, Args&&... args): m_ctx(std::make_shared<basic_context<T, Err>>())
        {
            std::forward<Fn>(exec)(m_ctx, std::forward<Args>(args)...);
        }

        template <typename Fn, typename... Args>
        explicit basic_op_handle(std::shared_ptr<detail::context_base> parent, Fn&& exec, Args&&... args)
            : m_ctx(std::make_shared<basic_context<T, Err>>(parent))
        {
            std::forward<Fn>(exec)(m_ctx, std::forward<Args>(args)...);
        }

        void cancel()
        {
            m_ctx->cancel();
        }

        template <typename Fn>
        auto then(Fn&& fn)
        {
            if constexpr (std::is_void_v<T>)
            {
                using info = continuation<Fn, std::tuple<>>;
                using ret_t = typename info::ret_type;

                return basic_op_handle<ret_t, Err>(
                        std::static_pointer_cast<detail::context_base>(m_ctx),
                        [this, &fn](basic_context_ptr<ret_t, Err> ctx)
                        {
                            m_ctx->set_continuation(
                                    info::template deferred<ret_t, Err, Fn>(ctx, std::forward<Fn>(fn)),
                                    default_skip_failcont<Err>(ctx));
                        });
            }
            else
            {
                using info = continuation<Fn, std::tuple<T&&>>;
                using ret_t = typename info::ret_type;

                return basic_op_handle<ret_t, Err>(
                        std::static_pointer_cast<detail::context_base>(m_ctx),
                        [this, &fn](basic_context_ptr<ret_t, Err> ctx)
                        {
                            m_ctx->set_continuation(
                                    info::template deferred<ret_t, Err, Fn, T&&>(ctx, std::forward<Fn>(fn)),
                                    default_skip_failcont<Err>(ctx));
                        });
            }
        }

        template <typename SuccCb, typename FailCb>
        auto then(SuccCb&& s, FailCb&& f)
        {
            using f_info = continuation<FailCb, std::tuple<Err&&>>;

            if constexpr (std::is_void_v<T>)
            {
                using s_info = continuation<SuccCb, std::tuple<>>;
                using ret_t = typename s_info::ret_type;

                return basic_op_handle<ret_t, Err>(
                        std::static_pointer_cast<detail::context_base>(m_ctx),
                        [this, &s, &f](basic_context_ptr<ret_t, Err> ctx)
                        {
                            m_ctx->set_continuation(
                                    s_info::template deferred<ret_t, Err, SuccCb>(ctx, std::forward<SuccCb>(s)),
                                    f_info::template deferred<ret_t, Err, FailCb, Err&&>(ctx, std::forward<FailCb>(f)));
                        });
            }
            else
            {
                using s_info = continuation<SuccCb, std::tuple<T&&>>;
                using ret_t = typename s_info::ret_type;

                return basic_op_handle<ret_t, Err>(
                        std::static_pointer_cast<detail::context_base>(m_ctx),
                        [this, &s, &f](basic_context_ptr<ret_t, Err> ctx)
                        {
                            m_ctx->set_continuation(
                                    s_info::template deferred<ret_t, Err, SuccCb, T&&>(ctx, std::forward<SuccCb>(s)),
                                    f_info::template deferred<ret_t, Err, FailCb, Err&&>(ctx, std::forward<FailCb>(f)));
                        });
            }
        }

        template <typename Fn>
        auto on_failure(Fn&& fn)
        {
            using info = continuation<Fn, std::tuple<Err&&>>;

            return basic_op_handle<void, Err>(
                    std::static_pointer_cast<detail::context_base>(m_ctx),
                    [this, &fn](basic_context_ptr<void, Err> ctx)
                    {
                        m_ctx->set_continuation(
                                default_skip_cont<T>(ctx),
                                info::template deferred<void, Err, Fn, Err&&>(ctx, std::forward<Fn>(fn)));
                    });
        }

    private:
        basic_context_ptr<T, Err> m_ctx;
    };
}
