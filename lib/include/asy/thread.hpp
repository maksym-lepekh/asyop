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

namespace asy::thread::detail
{
    template <typename F>
    auto fy_func(F&& f)
    {
        using ret_t = std::invoke_result_t<F>;

        auto origin_id = std::this_thread::get_id();

        return asy::op([fn = std::forward<F>(f), origin_id](asy::context<ret_t> ctx) mutable
        {
            std::thread([fn = std::forward<F>(fn), ctx, origin_id]() mutable
            {
                asy::executor::get().schedule_execution([ctx, ret = fn()]() mutable
                {
                    ctx->async_success(std::move(ret));
                }, origin_id);
            }).detach();
        });
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
        if constexpr (asy::tt::specialization_of<std::future, std::decay_t<F>>::value)
        {
            return detail::fy_future(std::forward<F>(f));
        }
        else
        {
            return detail::fy_func(std::forward<F>(f));
        }
    }
}
