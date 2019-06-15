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

TEST_CASE("Op from std::function", "[asio]")
{
    auto io = asio::io_service{};
    auto timer = asio::steady_timer{io, 50ms};

    timer.async_wait([](const asio::error_code& err){
        if (!err) FAIL("Timeout");
    });

    asy::this_thread::set_event_loop(io);

    SECTION("Simple")
    {
        auto f = std::function<std::string()>{[](){ return "abc"; }};

        asy::op(f).then([&](auto&& s){
            CHECK(s == "abc");
            timer.cancel();
        });

        io.run();
    }

    SECTION("Areturn")
    {
        auto f = std::function<asy::op_handle<std::string>()>{[](){ return asy::op<std::string>("abc"); }};

        asy::op(f).then([&](auto&& s){
            CHECK(s == "abc");
            timer.cancel();
        });

        io.run();
    }

    SECTION("Async")
    {
        auto f = std::function<void(asy::context<std::string>)>{[](auto ctx){ ctx->async_return("abc"); }};

        asy::op(f).then([&](auto&& s){
            CHECK(s == "abc");
            timer.cancel();
        });

        io.run();
    }
}

TEST_CASE("Op pass-through", "[asio]")
{
    auto io = asio::io_service{};
    auto timer = asio::steady_timer{io, 50ms};

    timer.async_wait([](const asio::error_code& err){
        if (!err) FAIL("Timeout");
    });

    asy::this_thread::set_event_loop(io);

    asy::op(asy::op<int>(42)).then([&](int&& input){
        CHECK(input == 42);
        timer.cancel();
    });

    io.run();
}

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
            CHECK_FALSE( (op1 && op2 && op3 && op4 && op5) );
            op1 = true;
            ctx->async_return(42);
        });
    })
    .then([&](auto&& val){
        CHECK_FALSE( (op2 && op3 && op4 && op5) );
        CHECK(op1);
        CHECK(val == 42);
        op2 = true;
        return val * 2;
    })
    .then([&](auto&& val){
        CHECK_FALSE( (op3 && op4 && op5) );
        CHECK( (op1 && op2) );
        CHECK(val == 84);
        op3 = true;
        return val * 2;
    })
    .on_failure([&](auto&& err){
        CHECK_FALSE( (op4 && op5) );
        CHECK( (op1 && op2 && op3) );
        op4 = true;
    })
    .then([&](){
        CHECK_FALSE( (op4 && op5) );
        CHECK( (op1 && op2 && op3) );
        op5 = true;
        return 15;
    });

    io.run();

    CHECK_FALSE(op4);
    CHECK( (op1 && op2 && op3 && op5) );
}

struct move_test_t
{
    int val;
    explicit move_test_t(int v) : val(v) {}
    move_test_t(move_test_t&& o) noexcept : val(o.val) {}
    move_test_t(const move_test_t& o) : val(o.val) { FAIL("Copy constructor"); }
    move_test_t& operator=(move_test_t&& o) noexcept { val = o.val; return *this; }
    move_test_t& operator=(const move_test_t& o) { val = o.val; FAIL("Copy assignment"); return *this; }
};

TEST_CASE("No copy", "[asio]")
{
    auto io = asio::io_service{};
    auto timer = asio::steady_timer{io, 200ms};

    timer.async_wait([](const asio::error_code& err){
        if (!err) FAIL("Timeout");
    });

    asy::this_thread::set_event_loop(io);

    SECTION("Then after return")
    {
        asy::op([]() { return move_test_t{42}; })
        .then([&](auto&& t) {
            CHECK(t.val == 42);
            timer.cancel();
        });

        io.run();
    }

    SECTION("Return after then")
    {
        asy::sleep(10ms)
        .then([]() { return move_test_t{42}; })
        .then([&](auto&& t) {
            CHECK(t.val == 42);
            timer.cancel();
        });

        io.run();
    }
}
