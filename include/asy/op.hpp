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
        using ctx_t = context<T>;
        using ctx_arg_t = std::shared_ptr<ctx_t>;
        using fn_t = std::function<void(ctx_arg_t)>;
        using success_type = T;
        using failure_type = std::error_code;
        using success_cb_t = std::function<void(success_type&)>;
        using failure_cb_t = std::function<void(failure_type&)>;

        explicit op_handle(fn_t exec)
        {
            m_ctx = std::make_shared<ctx_t>();
            exec(m_ctx);
        }

        void cancel()
        {
            m_ctx->cancel();
        }

        void then(success_cb_t s, failure_cb_t f = {})
        {
            m_ctx->then(std::move(s), std::move(f));
        }

    private:
        std::shared_ptr<ctx_t> m_ctx;
    };

    template <typename Ret>
    auto op(std::function<void(std::shared_ptr<context<Ret>>)> fn)
    {
        return op_handle<Ret>{std::move(fn)};
    }

    /*auto op(std::function<void(std::shared_ptr<context<void>>)> fn)
    {
        return op_handle<void>{std::move(fn)};
    }*/
}
