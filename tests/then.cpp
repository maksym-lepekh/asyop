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
#include <string>

using namespace std::literals;

TEST_CASE("asy::op then", "[asio]")
{
    auto io = asio::io_service{};
    auto timer = asio::steady_timer{io, 50ms};

    timer.async_wait([](const asio::error_code& err){
        if (!err) FAIL("Timeout");
    });

    asy::this_thread::set_event_loop(io);

    SECTION("Simple continuation")
    {
        asy::op<int>(42).then([&](int&& input){
            REQUIRE( input == 42 );
            timer.cancel();
        });

        io.run();
    }

    SECTION("Areturn continuation")
    {
        asy::op<int>().then([&](int&&){
            timer.cancel();
            return asy::op<double>();
        });

        io.run();
    }

    SECTION("Async continuation")
    {
        asy::op<int>().then([&](asy::context<double> ctx, int&&){
            timer.cancel();
            ctx->async_return(42.0);
        });

        io.run();
    }

    SECTION("Mixed chain")
    {
        asy::op<int>().then([&](asy::context<double> ctx, int&& input){
            REQUIRE( input == 0 );
            ctx->async_return(42.0);
        })
        .then([&](double && input){
            REQUIRE( input == 42.0 );
            return asy::op<std::string>();
        })
        .then([&](std::string&& input){
            REQUIRE( input.empty() );
            timer.cancel();
        });

        io.run();
    }

    SECTION("Chain with fail")
    {
        asy::op<int>().then([&](asy::context<double> ctx, int&& input){
            REQUIRE( input == 0 );
            ctx->async_return(std::make_error_code(std::errc::address_in_use));
        })
        .then([&](double&& input){
            FAIL("Wrong path");
            return asy::op<std::string>();
        },
        [&](std::error_code&& err){
            REQUIRE( err == std::make_error_code(std::errc::address_in_use) );
        })
        .then([&](std::string&& input){
            REQUIRE( input.empty() );
            timer.cancel();
        });

        io.run();
    }

    SECTION("Chain with on_failure")
    {
        asy::op<int>(1337)
        .then([&](asy::context<double> ctx, int&& input){
            REQUIRE( input == 1337 );
            ctx->async_return(std::make_error_code(std::errc::address_in_use));
        })
        .on_failure([](std::error_code&& err){
            REQUIRE( err == std::make_error_code(std::errc::address_in_use) );
        })
        .then([&](){ timer.cancel(); });

        io.run();
    }
}
