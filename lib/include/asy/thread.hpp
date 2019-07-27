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

#include <asy/op.hpp>
#include <type_traits>
#include <thread>
#include <future>
#include <memory>
#include <utility>

namespace asy::thread::detail
{
    template <typename F>
    auto fy_func(F&& f)
    {
        using ret_t = std::invoke_result_t<F>;

        auto origin_id = std::this_thread::get_id();
        auto thread_handle = std::thread{};

        auto h = asy::op([fn = std::forward<F>(f), origin_id, &thread_handle](asy::context<ret_t> ctx) mutable
        {
            thread_handle = std::thread([fn = std::forward<F>(fn), ctx, origin_id]() mutable
            {
                asy::executor::schedule_execution([ctx, ret = fn()]() mutable
                {
                    ctx->async_success(std::move(ret));
                }, origin_id);
            });
        });

        return std::pair(std::move(h), std::move(thread_handle));
    }

    template <typename T>
    auto fy_future(std::future<T>&& fut)
    {
        return fy_func([fut = std::move(fut)]() mutable { return fut.get(); });
    }
}

namespace asy::thread
{
    /// Convert a blocking operation into an asynchronous operation. A new thread is started for invocation.
    /// Also supports extraction of the result from the `std::future`
    ///
    /// \param f A functor that represents a computation, or a future object
    /// \return Operation handle
    template <typename F>
    auto fy(F&& f)
    {
        if constexpr (asy::util::specialization_of<std::future, std::decay_t<F>>::value)
        {
            auto&& [op, thr] = detail::fy_future(std::forward<F>(f));
            thr.detach();
            return op;
        }
        else
        {
            auto&& [op, thr] = detail::fy_func(std::forward<F>(f));
            thr.detach();
            return op;
        }
    }

    /// Convert a blocking operation into an asynchronous operation. A new thread is started for invocation.
    /// Also supports extraction of the result from the `std::future`
    ///
    /// \param f A functor that represents a computation, or a future object
    /// \param t_handle [out] handle of the created thread
    /// \return Operation handle
    template <typename F>
    auto fy(F&& f, std::thread& t_handle)
    {
        if constexpr (asy::util::specialization_of<std::future, std::decay_t<F>>::value)
        {
            auto&& [op, thr] = detail::fy_future(std::forward<F>(f));
            t_handle = std::move(thr);
            return op;
        }
        else
        {
            auto&& [op, thr] = detail::fy_func(std::forward<F>(f));
            t_handle = std::move(thr);
            return op;
        }
    }
}
