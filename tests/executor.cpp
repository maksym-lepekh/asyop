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

#include <catch2/catch.hpp>
#include <asy/core/executor.hpp>
#include <thread>
#include <atomic>
#include <condition_variable>
#include "barrier.hpp"

using namespace std::literals;


TEST_CASE("executor::should_sync multi", "[core]")
{
    auto barr = barrier{2};

    auto test1 = std::atomic_bool{false};
    auto test2 = std::atomic_bool{false};

    auto main_id = std::this_thread::get_id();
    asy::executor::set_impl(main_id, [](auto){}, true);

    auto worker_id = std::atomic<decltype(main_id)>{};
    auto worker = std::thread{[&]{
        worker_id = std::this_thread::get_id();
        asy::executor::set_impl(worker_id, [](auto){}, false);

        test1 = asy::executor::should_sync(main_id);
        test2 = !asy::executor::should_sync(worker_id);

        barr.wait();
        barr.wait();
    }};

    barr.wait();

    CHECK(asy::executor::should_sync(main_id));
    CHECK(!asy::executor::should_sync(worker_id));
    CHECK(test1);
    CHECK(test2);

    barr.wait();

    if (worker.joinable())
    {
        worker.join();
    }
}
