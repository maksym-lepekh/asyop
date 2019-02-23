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
#include <pthread.h>

namespace asy::thread::detail
{
    template <typename F>
    auto fy_func(F&& f)
    {
        using ret_t = std::invoke_result_t<F>;

        auto thr = std::make_shared<std::thread>();
        auto origin_id = std::this_thread::get_id();

        auto h = asy::op([fn = std::forward<F>(f), &thr, origin_id](asy::context<ret_t> ctx) mutable
        {
            *thr = std::thread([fn = std::forward<F>(fn), ctx, origin_id]()
            {
                asy::executor::get().schedule_execution([ctx, ret = fn()]() mutable
                {
                    ctx->async_success(std::move(ret));
                }, origin_id);
            });
            thr->detach();
        });

        return add_cancel(h, [thr]
        {
            if (thr->joinable()) pthread_cancel(thr->native_handle());
        });
    }

    template <typename T>
    auto fy_future(std::future<T>&& fut)
    {
        auto sh_fut = std::make_shared<std::future<T>>(std::move(fut));
        return fy_func([sh_fut]{ return std::move(sh_fut->get()); });
    }
}

namespace asy::thread
{
    template <typename F>
    auto fy(F&& f)
    {
        if constexpr (asy::detail::specialization_of<std::future, std::decay_t<F>>::value)
        {
            return detail::fy_future(std::forward<F>(f));
        }
        else
        {
            return detail::fy_func(std::forward<F>(f));
        }
    }
}
