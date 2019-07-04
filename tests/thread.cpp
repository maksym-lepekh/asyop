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
#include <asy/thread.hpp>
#include <chrono>
#include <string>
#include <atomic>

using namespace std::literals;


TEST_CASE("Threadify", "[asio]")
{
    auto io = asio::io_service{};
    auto timer = asio::steady_timer{io, 200ms};

    timer.async_wait([](const asio::error_code& err) {
        if (!err) FAIL("Timeout");
    });

    asy::this_thread::set_event_loop(io);

    SECTION("Simple case")
    {
        auto main_id = std::this_thread::get_id();
        auto is_other_thread = std::atomic_bool{false};

        asy::thread::fy([main_id, &is_other_thread](){
            is_other_thread = (main_id != std::this_thread::get_id());
            std::this_thread::sleep_for(5ms);
            return 42;
        })
        .then([&](int&& input) {
            CHECK(is_other_thread);
            CHECK(input == 42);
            timer.cancel();
        });

        io.run();
    }

    SECTION("std::future")
    {
        asy::thread::fy(std::async(std::launch::async, []{
            std::this_thread::sleep_for(5ms);
            return 42;
        }))
        .then([&](int&& input) {
            CHECK(input == 42);
            timer.cancel();
        });

        io.run();
    }

    SECTION("std::future, with handle")
    {
        auto thr = std::thread{};

        asy::thread::fy(std::async(std::launch::async, []{
            std::this_thread::sleep_for(5ms);
            return 42;
        }), thr)
        .then([&](int&& input) {
            CHECK(input == 42);
            timer.cancel();
        });

        io.run();
        if (thr.joinable()) thr.join();
    }

    SECTION("Cancel thread, with handle")
    {
        auto main_id = std::this_thread::get_id();
        auto is_other_thread = std::atomic_bool{false};
        auto thr = std::thread{};

        auto handle = asy::thread::fy([main_id, &is_other_thread](){
            is_other_thread = (main_id != std::this_thread::get_id());
            std::this_thread::sleep_for(50ms);
            return 42;
        }, thr)
        .then([&](int&& input) { FAIL("Wrong path"); }, [&](auto err){
            CHECK( err == std::make_error_code(std::errc::operation_canceled) );
            timer.cancel();
        });

        io.post([&](){
            handle.cancel();
        });

        io.run();
        if (thr.joinable()) thr.join();
        CHECK(is_other_thread);
    }

    SECTION("Cancel thread")
    {
        auto main_id = std::this_thread::get_id();
        auto is_other_thread = std::atomic_bool{false};

        auto handle = asy::thread::fy([main_id, &is_other_thread](){
            is_other_thread = (main_id != std::this_thread::get_id());
            std::this_thread::sleep_for(50ms);
            return 42;
        })
        .then([&](int&& input) { FAIL("Wrong path"); }, [&](auto err){
            CHECK( err == std::make_error_code(std::errc::operation_canceled) );
            timer.cancel();
        });

        io.post([&](){
            handle.cancel();
        });

        io.run();
        std::this_thread::sleep_for(100ms);
        CHECK(is_other_thread);
    }
}
