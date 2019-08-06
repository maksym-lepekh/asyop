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

#include "core/basic_context.hpp"
#include "core/basic_op_handle.hpp"
#include "common/simple_continuation.hpp"
#include "common/voe_continuation.hpp"
#include "common/aret_continuation.hpp"
#include "common/ctx_continuation.hpp"
#include "basic_ops.hpp"
#include "basic_op.hpp"

#include <system_error>


namespace asy
{
    template <> struct error_traits<std::error_code>
    {
        static std::error_code get_canceled()
        {
            return std::make_error_code(std::errc::operation_canceled);
        }
    };

    /// Default (std::error_code) specialisation of `op_handle`
    template <typename T>
    using op_handle = basic_op_handle<T, std::error_code>;

    /// Default (std::error_code) specialisation of `context_ptr`
    template <typename T>
    using context = basic_context_ptr<T, std::error_code>;

    /// Default (std::error_code) specialisation of `op()`
    template <typename F, typename... Args>
    decltype(auto) op(F&& fn, Args&&... args)
    {
        return basic_op<std::error_code>(std::forward<F>(fn), std::forward<Args>(args)...);
    }

    /// Default (std::error_code) specialisation of `op()`
    template <typename Ret>
    decltype(auto) op()
    {
        return basic_op<Ret, std::error_code>();
    }

    /// Default (std::error_code) specialisation of `when_all()`
    template <typename... Fs>
    decltype(auto) when_all(Fs&&...fs)
    {
        return basic_when_all<std::error_code>(std::forward<Fs>(fs)...);
    }

    /// Default (std::error_code) specialisation of `when_success()`
    template <typename... Fs>
    decltype(auto) when_success(Fs&&...fs)
    {
        return basic_when_success<std::error_code>(std::forward<Fs>(fs)...);
    }

    /// Default (std::error_code) specialisation of `when_any()`
    template <typename... Fs>
    decltype(auto) when_any(Fs&&...fs)
    {
        return basic_when_any<std::error_code>(std::forward<Fs>(fs)...);
    }
}
