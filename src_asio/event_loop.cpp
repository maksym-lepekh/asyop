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
#include <optional>

namespace
{
    thread_local asio::io_service* svc = nullptr;
}

void asy::this_thread::set_event_loop(::asio::io_service& s)
{
    svc = &s;
    detail::post_impl = [](detail::posted_fn fn){
        svc->post(std::move(fn));
    };
}

asio::io_service& asy::this_thread::get_event_loop()
{
    return *svc;
}
