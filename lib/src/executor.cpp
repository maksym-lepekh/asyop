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
#include <asy/core/executor.hpp>
#include <map>
#include <optional>
#include <mutex>
#include <utility>
#include <cassert>

namespace
{
    using reg_rec_t = std::pair<asy::executor::impl_t, bool>;

    auto registry = std::map<std::thread::id, reg_rec_t>{};
    auto reg_mutex = std::mutex{};
    thread_local auto this_impl = std::optional<reg_rec_t>{};
}

void asy::executor::schedule_execution(asy::executor::fn_t fn, std::thread::id id)
{
    if (this_impl && (id == std::this_thread::get_id()))
    {
        std::invoke(this_impl->first, std::move(fn));
    }
    else
    {
        auto guard = std::lock_guard{reg_mutex};
        assert(registry.find(id) != registry.end());
        std::invoke(registry[id].first, std::move(fn));
    }
}

bool asy::executor::should_sync(std::thread::id id) noexcept
{
    if (this_impl && (id == std::this_thread::get_id()))
    {
        return this_impl->second;
    }

    auto guard = std::lock_guard{reg_mutex};
    assert(registry.find(id) != registry.end());
    return registry[id].second;
}

void asy::executor::set_impl(std::thread::id id, asy::executor::impl_t impl, bool require_sync)
{
    if (id == std::this_thread::get_id())
    {
        this_impl = reg_rec_t{impl, require_sync};
    }

    auto guard = std::lock_guard{reg_mutex};
    registry[id] = { std::move(impl), require_sync };
}
