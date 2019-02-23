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
#include <asy/executor.hpp>
#include <map>
#include <utility>
#include <cassert>

namespace
{
    auto registry = std::map<std::thread::id, std::pair<asy::executor::impl_t, bool>>{};
}

asy::executor::executor() = default;

asy::executor& asy::executor::get() noexcept
{
    static executor inst;
    return inst;
}

void asy::executor::schedule_execution(asy::executor::fn_t fn, std::thread::id id) noexcept
{
    assert(registry.find(id) != registry.end());
    std::invoke(registry[id].first, std::move(fn));
}

bool asy::executor::should_sync(std::thread::id id) const noexcept
{
    assert(registry.find(id) != registry.end());
    return registry[id].second;
}

void asy::executor::set_impl(std::thread::id id, asy::executor::impl_t impl, bool require_sync)
{
    registry[id] = { std::move(impl), require_sync };
}
