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
#include <asy/event_loop_asio.hpp>
#include <chrono>
#include <tuple>
#include <functional>

using namespace std::literals;


TEST_CASE("Op with aux", "[asio]")
{
    auto io = asio::io_service{};
    auto timer = asio::steady_timer{io, 50ms};

    timer.async_wait([](const asio::error_code& err){
        if (!err) FAIL("Timeout");
    });

    asy::this_thread::set_event_loop(io);

    SECTION("Simple")
    {
        asy::op([](const std::string& s, int i){
            CHECK(s == "abc");
            CHECK(i == 42);
            return std::make_tuple(s, i);
        }, "abc", 42)
        .then([&](auto&& t){
            auto& [s, i] = t;
            CHECK(s == "abc");
            CHECK(i == 42);
            timer.cancel();
        });

        io.run();
    }

    SECTION("Areturn")
    {
        asy::op([](const std::string& s, int i){
            CHECK(s == "abc");
            CHECK(i == 42);
            return asy::op(std::make_tuple(s, i));
        }, "abc", 42)
        .then([&](auto&& t){
            auto& [s, i] = t;
            CHECK(s == "abc");
            CHECK(i == 42);
            timer.cancel();
        });

        io.run();
    }

    SECTION("Async")
    {
        asy::op([](asy::context<std::tuple<std::string, int>> ctx, const std::string& s, int i){
            CHECK(s == "abc");
            CHECK(i == 42);
            ctx->async_return(std::make_tuple(s, i));
        }, "abc", 42)
        .then([&](auto&& t){
            auto& [s, i] = t;
            CHECK(s == "abc");
            CHECK(i == 42);
            timer.cancel();
        });

        io.run();
    }
}

