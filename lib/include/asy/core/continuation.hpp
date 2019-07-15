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

#include <type_traits>
#include <utility>
#include "basic_context.hpp"

namespace asy
{
    /// Traits-like helper that defines a common usage interface for different continuatinos
    template <typename F, typename Sfinae = void>
    struct continuation : std::false_type
    {
        /// Invoke the functor and return op handle
        /// \note It is expected to be called as `c::template to_handle<Err>(f, args...)`
        ///
        /// \tparam Err Expected error type
        /// \param f Functor that matches supported signature
        /// \param args Functor call arguments (forwarded)
        /// \return asy::basic_op_handle of running computation
        template <typename Err, typename... Args>
        static auto to_handle(std::in_place_type_t<Err> /*err type*/, F&& /*f*/, Args&&... /*args*/)
        {
            static_assert(!std::is_void_v<Sfinae>, "Invalid continuation type");
        }


        /// Create callable that invokes the functor and forwards result to given context
        /// \note It is expected to be called as `c::deferred(ctx, f, args...)`, everything can be
        ///  deduced.
        ///
        /// \param ctx Context of outer operation
        /// \param f Functor
        /// \param args Functor arguments
        /// \return Callable
        template <typename T, typename Err, typename... Args>
        static auto deferred(asy::basic_context_ptr<T, Err> /*ctx*/, F&& /*f*/, Args&&... /*args*/)
        {
            static_assert(!std::is_void_v<Sfinae>, "Invalid continuation type");
        }
    };

    /// Generate a continuation that forwards successful result of previous operation
    ///
    /// \tparam Input Return type of the previous operation
    /// \param ctx Context of the generated continuation
    /// \return Continuation
    template <typename Input, typename Ctx>
    static auto default_skip_cont(Ctx ctx)
    {
        if constexpr (std::is_void_v<Input>)
        {
            return [ctx](){ ctx->async_success(); };
        }
        else
        {
            return [ctx](Input&& /*input*/){ ctx->async_success(); };
        }
    }

    /// Generate a continuation that forwards failure of previous operation
    ///
    /// \tparam Input Return type of the previous operation
    /// \param ctx Context of the generated continuation
    /// \return Continuation
    template <typename Input, typename Ctx>
    static auto default_skip_failcont(Ctx ctx)
    {
        return [ctx](Input&& err)
        {
            ctx->async_failure(std::move(err));
        };
    }
}
