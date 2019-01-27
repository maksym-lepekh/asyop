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

#include "context.hpp"
#include "detail/continuation_info.hpp"
#include "detail/continuations.hpp"

#include <memory>
#include <functional>


namespace asy
{
    template <typename T>
    class op_handle
    {
    public:
        using failure_type = std::error_code;
        using ctx_t = detail::context<T, failure_type>;

        template <typename Fn, typename... Args>
        explicit op_handle(Fn&& exec, Args&&... args): m_ctx(std::make_shared<ctx_t>())
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

            return op_handle<ret_t>([this, &fn](context<ret_t> ctx){
                m_ctx->set_continuation(
                        make_success_cont(std::forward<Fn>(fn), ctx),
                        detail::make_skip_failcont<failure_type>(ctx));
            });
        }

        template <typename SuccCb, typename FailCb>
        auto then(SuccCb&& s, FailCb f)
        {
            using info = detail::continuation_info<SuccCb, T>;
            static_assert(info::type != detail::cont_type::invalid, "Functor has unsupported type");
            using ret_t = typename info::ret_type;

            return op_handle<ret_t>([this, &s, &f](context<ret_t> ctx){
                m_ctx->set_continuation(
                        make_success_cont(std::forward<SuccCb>(s), ctx),
                        make_fail_cont(std::forward<FailCb>(f), ctx));
            });
        }

        template <typename Fn>
        auto on_failure(Fn&& fn)
        {
            return op_handle<void>([this, &fn](context<void> ctx){
                m_ctx->set_continuation(
                        detail::make_skip_cont<T>(ctx),
                        make_fail_cont(std::forward<Fn>(fn), ctx));
            });
        }

    private:
        template <typename Fn, typename Ctx>
        auto make_success_cont(Fn&& fn, Ctx ctx)
        {
            using info = detail::continuation_info<Fn, T>;
            static_assert(info::type != detail::cont_type::invalid, "Functor has unsupported type");
            using ret_t = typename info::ret_type;

            if constexpr (info::type == detail::cont_type::simple || info::type == detail::cont_type::ambiguous_simple)
            {
                return detail::make_simple_cont<T, ret_t>(std::forward<Fn>(fn), ctx);
            }
            else if constexpr (info::type == detail::cont_type::areturn || info::type == detail::cont_type::ambiguous_areturn)
            {
                return detail::make_areturn_cont<T, ret_t>(std::forward<Fn>(fn), ctx);
            }
            else
            {
                return detail::make_async_cont<T, ret_t, op_handle<ret_t>>(std::forward<Fn>(fn), ctx);
            }
        }

        template <typename Fn, typename Ctx>
        auto make_fail_cont(Fn&& fn, Ctx ctx)
        {
            using info = detail::continuation_info<Fn, failure_type>;
            static_assert(info::type != detail::cont_type::invalid, "Functor has unsupported type");

            if constexpr (info::type == detail::cont_type::simple || info::type == detail::cont_type::ambiguous_simple)
            {
                return detail::make_simple_failcont<failure_type>(std::forward<Fn>(fn), ctx);
            }
            else if constexpr (info::type == detail::cont_type::areturn || info::type == detail::cont_type::ambiguous_areturn)
            {
                return detail::make_areturn_failcont<failure_type>(std::forward<Fn>(fn), ctx);
            }
            else
            {
                return detail::make_async_failcont<failure_type, op_handle<void>>(std::forward<Fn>(fn), ctx);
            }
        }

        std::shared_ptr<ctx_t> m_ctx;
    };

    template <typename F, typename... Args>
    auto op(F&& fn, Args&&... args)
    {
        using info = detail::continuation_info<F, void>;

        if constexpr (info::type == detail::cont_type::invalid && sizeof...(Args) == 0)
        {
            using ret_t = std::decay_t<F>;
            return op_handle<ret_t>{[](context<ret_t> ctx, F&& init){
                ctx->async_return(std::forward<F>(init));
            }, std::forward<F>(fn)};
        }
        else
        {
            static_assert(info::type != detail::cont_type::invalid, "Functor has unsupported type");
            using ret_t = typename info::ret_type;

            if constexpr (info::type == detail::cont_type::simple || info::type == detail::cont_type::ambiguous_simple)
            {
                return op_handle<ret_t>{[&fn](context<void> ctx){
                    ctx->async_return(fn());
                }, std::forward<Args>(args)...};
            }
            else if constexpr (info::type == detail::cont_type::areturn || info::type == detail::cont_type::ambiguous_areturn)
            {
                return fn(std::forward<Args>(args)...);
            }

            return op_handle<ret_t>{fn, std::forward<Args>(args)...};
        }
    }

    template <typename Ret>
    auto op()
    {
        return op_handle<Ret>{[](context<Ret> ctx){
            ctx->async_return();
        }};
    }

}
