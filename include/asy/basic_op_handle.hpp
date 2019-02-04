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
#include "detail/continuation_info.hpp"
#include "detail/continuations.hpp"

namespace asy
{
    template <typename T, typename Err>
    class basic_op_handle
    {
    public:
        template <typename Fn, typename... Args>
        explicit basic_op_handle(Fn&& exec, Args&&... args): m_ctx(std::make_shared<basic_context<T, Err>>())
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
            using info = detail::continuation_info<Fn, T>;
            static_assert(info::type != detail::cont_type::invalid, "Functor has unsupported type");
            using ret_t = typename info::ret_type;

            return basic_op_handle<ret_t, Err>([this, &fn](basic_context_ptr<ret_t, Err> ctx){
                m_ctx->set_continuation(
                        detail::make_success_cont<T, Err>(std::forward<Fn>(fn), ctx),
                        detail::make_skip_failcont<Err>(ctx));
            });
        }

        template <typename SuccCb, typename FailCb>
        auto then(SuccCb&& s, FailCb f)
        {
            using info = detail::continuation_info<SuccCb, T>;
            static_assert(info::type != detail::cont_type::invalid, "Functor has unsupported type");
            using ret_t = typename info::ret_type;

            return basic_op_handle<ret_t, Err>([this, &s, &f](basic_context_ptr<ret_t, Err> ctx){
                m_ctx->set_continuation(
                        detail::make_success_cont<T, Err>(std::forward<SuccCb>(s), ctx),
                        detail::make_fail_cont<T, Err>(std::forward<FailCb>(f), ctx));
            });
        }

        template <typename Fn>
        auto on_failure(Fn&& fn)
        {
            return basic_op_handle<void, Err>([this, &fn](basic_context_ptr<void, Err> ctx){
                m_ctx->set_continuation(
                        detail::make_skip_cont<T>(ctx),
                        detail::make_fail_cont<T, Err>(std::forward<Fn>(fn), ctx));
            });
        }

    private:
        basic_context_ptr<T, Err> m_ctx;
    };
}
