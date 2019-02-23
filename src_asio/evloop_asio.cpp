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
#include <asy/evloop_asio.hpp>
#include <asy/op.hpp>
#include <asy/executor.hpp>
#include <optional>
#include <cassert>
#include <map>

namespace
{
    auto registry = std::map<std::thread::id, asio::io_service*>{};
}

void asy::this_thread::v1::set_event_loop(::asio::io_service& s)
{
    auto id = std::this_thread::get_id();
    registry[id] = &s;

    asy::executor::get().set_impl(
            id,
            [id](asy::executor::fn_t fn)
            {
                registry[id]->post(std::move(fn));
            },
            false);
}

asio::io_service& asy::this_thread::v1::get_event_loop()
{
    auto id = std::this_thread::get_id();
    assert(registry.find(id) != registry.end());
    return *registry[id];
}
