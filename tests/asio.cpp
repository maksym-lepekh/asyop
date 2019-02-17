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
#include <asio.hpp>
#include <asy/op.hpp>
#include <asy/evloop_asio.hpp>
#include <chrono>
#include <string>

using namespace std::literals;

TEST_CASE("asiofy", "[asio]")
{
    auto io = asio::io_service{};
    auto fail_timer = asio::steady_timer{io, 50ms};

    fail_timer.async_wait([](const asio::error_code& err){
        if (!err) FAIL("Timeout");
    });

    asy::this_thread::set_event_loop(io);

    SECTION("Timer success")
    {
        auto timer = asio::steady_timer{io, 5ms};

        asy::asio::fy<>([&](auto&& handler){ timer.async_wait(handler); })
        .then([&](){ fail_timer.cancel(); });

        io.run();
    }

    SECTION("Timer cancelled")
    {
        auto timer = asio::steady_timer{io, 5ms};

        asy::asio::fy<>([&](auto&& handler){ timer.async_wait(handler); })
        .on_failure([&](asio::error_code e)
        {
            CHECK(e == make_error_code(asio::error::operation_aborted));
            fail_timer.cancel();
        });

        timer.cancel();
        io.run();
    }
}
