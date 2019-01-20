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
        using ctx_arg_t = std::shared_ptr<ctx_t>;
        using fn_t = std::function<void(ctx_arg_t)>;

        using success_cb_t = typename ctx_t::success_cb_t;
        using failure_cb_t = typename ctx_t::failure_cb_t;

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
                return [fn = std::forward<Fn>(fn), ctx](T&& input){
                    // todo void unput
                    op_handle<ret_t>(fn, std::move(input)).then(
                        [ctx](ret_t&& output){ ctx->async_return(std::move(output)); },
                        [ctx](auto&& err){ ctx->async_return(std::move(err)); });
                };
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
                return [fn = std::forward<Fn>(fn), ctx](T&& input){
                    // todo void input
                    op_handle<void>(fn, std::move(input)).then(
                        [ctx](){ ctx->async_return(); },
                        [ctx](auto&& err){ ctx->async_return(err); });
                };
            }
        }

        std::shared_ptr<ctx_t> m_ctx;
    };

    template <typename F, typename... Args>
    auto op(F&& fn, Args&&... args)
    {
        using info = detail::continuation_info<F, void>;
        static_assert(info::type != detail::cont_type::invalid, "Functor has unsupported type");
        using ret_t = typename info::ret_type;

        if constexpr (info::type == detail::cont_type::simple || info::type == detail::cont_type::ambiguous_simple)
        {
            return op_handle<ret_t>{[&fn](context<void> ctx){
                ctx->async_return(fn());
            }};
        }
        else if constexpr (info::type == detail::cont_type::areturn || info::type == detail::cont_type::ambiguous_areturn)
        {
            return fn();
        }

        return op_handle<ret_t>{fn};
    }

    template <typename Ret>
    auto op()
    {
        return op_handle<Ret>{[](context<Ret> ctx){
            ctx->async_return();
        }};
    }

}
