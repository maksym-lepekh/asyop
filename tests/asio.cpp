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

        asy::fy<>([&](auto&& handler){ timer.async_wait(handler); })
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

TEST_CASE("sleep", "[asio]")
{
    using namespace std::literals;

    auto io = asio::io_service{};
    auto fail_timer = asio::steady_timer{io, 50ms};

    fail_timer.async_wait([](const asio::error_code& err){
        if (!err) FAIL("Timeout");
    });

    asy::this_thread::set_event_loop(io);

    SECTION("Timer success")
    {
        auto now = std::chrono::steady_clock::now();

        asy::asio::sleep(5ms).then([&]()
        {
            auto after = std::chrono::steady_clock::now();
            CHECK((now + 5ms) <= after);
            fail_timer.cancel();
        });

        io.run();
    }

    SECTION("Timer cancelled")
    {
        auto now = std::chrono::steady_clock::now();
        auto h = asy::asio::sleep(5ms);

        h.then([](){ FAIL("Wrong path"); }, [&](asio::error_code e)
        {
            auto after = std::chrono::steady_clock::now();
            CHECK((now + 5ms) > after);
            CHECK(e == make_error_code(std::errc::operation_canceled));
            fail_timer.cancel();
        });

        h.cancel();
        io.run();
    }
}

TEST_CASE("timed_op", "[asio]")
{
    using namespace std::literals;

    auto io = asio::io_service{};
    auto fail_timer = asio::steady_timer{io, 50ms};

    fail_timer.async_wait([](const asio::error_code& err){
        if (!err) FAIL("Timeout");
    });

    asy::this_thread::set_event_loop(io);

    SECTION("Success")
    {
        auto now = std::chrono::steady_clock::now();

        asy::asio::timed_op(10ms, asy::asio::sleep(5ms).then([]{ return 42; }))
        .then([&](int&& input){
            CHECK(input == 42);
            fail_timer.cancel();
        });

        io.run();
    }

    SECTION("Time out")
    {
        auto now = std::chrono::steady_clock::now();

        asy::timed_op(5ms, asy::sleep(10ms).then([]{ return 42; }))
        .on_failure([&](auto&& err){
            CHECK(err == make_error_code(std::errc::timed_out));
            fail_timer.cancel();
        });

        io.run();
    }

    SECTION("Cancel")
    {
        auto now = std::chrono::steady_clock::now();

        auto h = asy::timed_op(5ms, asy::sleep(10ms).then([]{ return 42; }))
                .on_failure([&](auto&& err){
                    CHECK(err == make_error_code(std::errc::operation_canceled));
                    fail_timer.cancel();
                });

        asy::asio::sleep(1ms).then([&]{ h.cancel(); });
        io.run();
    }
}

TEST_CASE("adapt", "[asio]")
{
    auto io = asio::io_service{};
    auto fail_timer = asio::steady_timer{io, 50ms};

    fail_timer.async_wait([](const asio::error_code& err){
        if (!err) FAIL("Timeout");
    });

    asy::this_thread::set_event_loop(io);

    SECTION("Timer success")
    {
        auto timer = asio::steady_timer{io, 15ms};

        timer.async_wait(asy::adapt).then([&](){ fail_timer.cancel(); });

        io.run();
    }

    SECTION("Timer cancelled")
    {
        auto timer = asio::steady_timer{io, 15ms};

        timer.async_wait(asy::adapt)
        .on_failure([&](asio::error_code e)
        {
            CHECK(e == make_error_code(asio::error::operation_aborted));
            fail_timer.cancel();
        });

        timer.cancel();
        io.run();
    }
}
