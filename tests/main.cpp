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


TEST_CASE("Chaining with fail", "[asio]")
{
    auto io = asio::io_service{};
    auto timer = asio::steady_timer{io, 1ms};

    asy::this_thread::set_event_loop(io);

    bool op1 = false;
    bool op2 = false;
    bool op3 = false;
    bool op4 = false;
    bool op5 = false;

    asy::op([&](asy::context<int> ctx){
        timer.async_wait([&, ctx](auto& ec) {
            REQUIRE_FALSE( (op1 && op2 && op3 && op4 && op5) );
            op1 = true;
            ctx->async_return(42);
        });
    })
    .then([&](auto&& val){
        REQUIRE_FALSE( (op2 && op3 && op4 && op5) );
        REQUIRE(op1);
        REQUIRE(val == 42);
        op2 = true;
        return val * 2;
    })
    .then([&](auto&& val){
        REQUIRE_FALSE( (op3 && op4 && op5) );
        REQUIRE( (op1 && op2) );
        REQUIRE(val == 84);
        op3 = true;
        return val * 2;
    })
    .on_failure([&](auto&& err){
        REQUIRE_FALSE( (op4 && op5) );
        REQUIRE( (op1 && op2 && op3) );
        op4 = true;
    })
    .then([&](){
        REQUIRE_FALSE( (op4 && op5) );
        REQUIRE( (op1 && op2 && op3) );
        op5 = true;
        return 15;
    });

    io.run();

    REQUIRE_FALSE(op4);
    REQUIRE( (op1 && op2 && op3 && op5) );
}
