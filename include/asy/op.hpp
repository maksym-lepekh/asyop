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

#include "context.hpp"

#include <memory>
#include <functional>


namespace asy
{
    template <typename T>
    class op_handle
    {
    public:
        using failure_type = std::error_code;

        using ctx_t = context<T, failure_type>;
        using ctx_arg_t = std::shared_ptr<ctx_t>;
        using fn_t = std::function<void(ctx_arg_t)>;

        using success_cb_t = typename ctx_t::success_cb_t;
        using failure_cb_t = typename ctx_t::failure_cb_t;

        explicit op_handle(fn_t exec)
        {
            m_ctx = std::make_shared<ctx_t>();
            exec(m_ctx);
        }

        void cancel()
        {
            m_ctx->cancel();
        }

        template <typename SuccCb>
        auto then(SuccCb&& s, failure_cb_t f = {})
        {
            using ret_t = typename detail::type_traits<T>::template cb_result<SuccCb>;
            using succ_cb_t = typename detail::type_traits<T>::template std_fun_t<SuccCb>;

            return op_handle<ret_t>([this, s = succ_cb_t{std::forward<SuccCb>(s)}, &f](auto ctx)
            {
                failure_cb_t fail_cont;
                if (f)
                {
                    f = make_fail_cont<ret_t>(std::move(f), ctx);
                }
                else
                {
                    f = make_fail_skip(ctx);
                }

                m_ctx->set_continuation(
                        make_succ_cont<ret_t>(std::move(s), ctx),
                        std::move(fail_cont)
                );
            });
        }

        auto on_failure(failure_cb_t f)
        {
            return op_handle<void>([this, &f](auto ctx)
            {
                m_ctx->set_continuation(
                        make_succ_skip<void>(ctx),
                        make_fail_cont<void>(std::move(f), ctx)
                );
            });
        }

    private:
        template <typename Ret, typename C>
        static auto make_succ_skip(C ctx)
        {
            return [ctx = ctx](auto&&... result)
            {
                if constexpr (std::is_void_v<Ret>)
                {
                    ctx->async_return();
                }
                else
                {
                    ctx->async_return(std::move(result)...);
                }
            };
        }

        template <typename Ret, typename S, typename C>
        static auto make_succ_cont(S user_cb, C ctx)
        {
            return [ctx = ctx, s_cb = std::move(user_cb)](auto&&... result)
            {
                if constexpr (std::is_void_v<Ret>)
                {
                    s_cb(std::move(result)...);
                    ctx->async_return();
                }
                else
                {
                    ctx->async_return(s_cb(std::move(result)...));
                }
            };
        }

        template <typename C>
        static auto make_fail_skip(C ctx)
        {
            return [ctx = ctx](failure_type&& err)
            {
                ctx->async_return(std::move(err));
            };
        }

        template <typename Ret, typename C>
        static auto make_fail_cont(failure_cb_t user_cb, C ctx)
        {
            return [ctx = ctx, f_cb = std::move(user_cb)](failure_type&& err)
            {
                f_cb(std::move(err));
                if constexpr (std::is_void_v<Ret>)
                {
                    ctx->async_return();
                }
                else
                {
                    ctx->async_return(Ret{});
                }
            };
        }

        std::shared_ptr<ctx_t> m_ctx;
    };

    template <typename Ret>
    auto op(std::function<void(std::shared_ptr<context<Ret, std::error_code>>)> fn)
    {
        return op_handle<Ret>{std::move(fn)};
    }
}
