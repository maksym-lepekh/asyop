// Copyright 2018 Maksym Lepekh
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

#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include <asio.hpp>

#include <asy/op.hpp>
#include <asy/event_loop_asio.hpp>
#include <optional>
#include <thread>
#include <chrono>
#include <iostream>

using namespace std::literals;

TEST_CASE( "Try unit tests", "[stub]" )
{
    auto opt = std::optional<int>{};

    REQUIRE(!(bool)opt);
}

TEST_CASE("Event loop", "[asio]")
{
    auto io = asio::io_service{};
    auto timer = asio::steady_timer{io, 2s};

    asy::this_thread::set_event_loop(io);

    asy::op<int>([&timer](auto ctx){
        std::cout << "op...\n";

        timer.async_wait([ctx](auto& ec) {
            ctx->async_return(42);
        });
    });

    io.run();
}

TEST_CASE("Event loop 2", "[asio]")
{
    auto io = asio::io_service{};
    auto timer = asio::steady_timer{io, 2s};

    asy::this_thread::set_event_loop(io);

    asy::op<int>([&timer](auto ctx){
        std::cout << "op...\n";

        timer.async_wait([ctx](auto& ec) {
            ctx->async_return(42);
        });
    }).then([](auto& val){
        std::cout << "returned " << val << "\n";
    });

    io.run();
}
