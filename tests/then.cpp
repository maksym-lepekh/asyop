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
#include "voe.hpp"

using namespace std::literals;

TEST_CASE("asy::op then", "[asio]")
{
    auto io = asio::io_service{};
    auto timer = asio::steady_timer{io, 200ms};

    timer.async_wait([](const asio::error_code& err){
        if (!err) FAIL("Timeout");
    });

    asy::this_thread::set_event_loop(io);

    SECTION("Simple continuation")
    {
        asy::op<int>(42).then([&](int&& input){
            CHECK( input == 42 );
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
            CHECK( input == 0 );
            ctx->async_return(42.0);
        })
        .then([&](double && input){
            CHECK( input == 42.0 );
            return asy::op<std::string>();
        })
        .then([&](std::string&& input){
            CHECK( input.empty() );
            timer.cancel();
        });

        io.run();
    }

    SECTION("Chain with fail")
    {
        asy::op<int>().then([&](asy::context<double> ctx, int&& input){
            CHECK( input == 0 );
            ctx->async_return(std::make_error_code(std::errc::address_in_use));
        })
        .then([&](double&& input){
            FAIL("Wrong path");
            return asy::op<std::string>();
        },
        [&](std::error_code&& err){
            CHECK( err == std::make_error_code(std::errc::address_in_use) );
        })
        .then([&](std::string&& input){
            CHECK( input.empty() );
            timer.cancel();
        });

        io.run();
    }

    SECTION("Chain with on_failure")
    {
        asy::op<int>(1337)
        .then([&](asy::context<double> ctx, int&& input){
            CHECK( input == 1337 );
            ctx->async_return(std::make_error_code(std::errc::address_in_use));
        })
        .on_failure([](std::error_code&& err){
            CHECK( err == std::make_error_code(std::errc::address_in_use) );
        })
        .then([&](){ timer.cancel(); });

        io.run();
    }

    SECTION("Simple continuation with ValueOrError")
    {
        asy::op<int>(42)
        .then([](int&& input)->voe<std::string>{
            CHECK( input == 42 );
            return std::to_string(input);
        })
        .then([&](std::string&& input){
            CHECK( input == "42" );
            timer.cancel();
        });

        io.run();
    }

    SECTION("Chain with on_failure and ValueOrError")
    {
        asy::op<int>(1337)
        .then([&](int&& input)->voe<std::string>{
            CHECK( input == 1337 );
            return std::make_error_code(std::errc::address_in_use);
        })
        .on_failure([](std::error_code&& err){
            CHECK( err == std::make_error_code(std::errc::address_in_use) );
        })
        .then([&](){ timer.cancel(); });

        io.run();
    }

    SECTION("Simple continuation with ValueOrNone")
    {
        asy::op<int>(42)
        .then([](int&& input)->von<std::string>{
            CHECK( input == 42 );
            return std::to_string(input);
        })
        .then([&](std::string&& input){
            CHECK( input == "42" );
            timer.cancel();
        });

        io.run();
    }

    SECTION("Chain with on_failure and ValueOrNone")
    {
        asy::op<int>(1337)
        .then([&](int&& input)->von<std::string>{
            CHECK( input == 1337 );
            return std::nullopt;
        })
        .on_failure([](std::error_code&& err){
            CHECK( err == std::error_code() );
        })
        .then([&](){ timer.cancel(); });

        io.run();
    }

    SECTION("Simple continuation with NoneOrError")
    {
        asy::op<int>(42)
        .then([](int&& input)->noe{
            CHECK( input == 42 );
            return {};
        })
        .then([&](){ timer.cancel(); });

        io.run();
    }

    SECTION("Chain with on_failure and NoneOrError")
    {
        asy::op<int>(1337)
        .then([](int&& input)->noe{
            CHECK( input == 1337 );
            return std::make_error_code(std::errc::address_in_use);
        })
        .on_failure([](std::error_code&& err){
            CHECK( err == std::make_error_code(std::errc::address_in_use) );
        })
        .then([&](){ timer.cancel(); });

        io.run();
    }

    SECTION("Cancel")
    {
        auto test_timer = asio::steady_timer{io, 50ms};

        auto handle = asy::op<int>(1337)
        .then([&](asy::context<double> ctx, int&& input){
            CHECK( input == 1337 );

            test_timer.async_wait([ctx](const asio::error_code& err){
                if (!err)
                {
                    ctx->async_success(42.0);
                }
            });
        })
        .on_failure([](std::error_code&& err){
            CHECK( err == std::make_error_code(std::errc::operation_canceled) );
        })
        .then([&](){
            test_timer.cancel();
            timer.cancel();
        });

        handle.cancel();
        io.run();
    }

    SECTION("Cancel, simple")
    {
        auto test_timer = asio::steady_timer{io, 50ms};
        auto finally_called = false;

        auto handle = asy::op([&](asy::context<int> ctx){
            test_timer.async_wait([ctx](const asio::error_code& err)
            {
                CHECK(err == make_error_code(asio::error::operation_aborted));
                if (!err) ctx->async_success(42);
            });
        }).then([](int&&){ FAIL("Wrong path"); }, [&](auto&& err){
            CHECK( err == std::make_error_code(std::errc::operation_canceled) );
            test_timer.cancel();
            timer.cancel();
        }).then([&](){
            finally_called = true;
        });

        handle.cancel();
        io.run();
        CHECK(finally_called);
    }

    SECTION("Cancel, on_failure")
    {
        auto test_timer = asio::steady_timer{io, 50ms};
        auto finally_called = false;

        auto handle = asy::op([&](asy::context<int> ctx){
            test_timer.async_wait([ctx](const asio::error_code& err){
                CHECK(err == make_error_code(asio::error::operation_aborted));
                if (!err) ctx->async_success(42);
            });
        }).on_failure([&](auto&& err){
            CHECK( err == std::make_error_code(std::errc::operation_canceled) );
            test_timer.cancel();
            timer.cancel();
        }).then([&](){
            finally_called = true;
        });

        handle.cancel();
        io.run();
        CHECK(finally_called);
    }

    SECTION("Cancel, double on_failure")
    {
        auto test_timer = asio::steady_timer{io, 50ms};
        auto finally_called = false;
        auto second_failure = false;

        auto handle = asy::op([&](asy::context<int> ctx){
            test_timer.async_wait([ctx](const asio::error_code& err){
                CHECK(err == make_error_code(asio::error::operation_aborted));
                if (!err) ctx->async_success(42);
            });
        }).on_failure([&](auto&& err){
            CHECK( err == std::make_error_code(std::errc::operation_canceled) );
            test_timer.cancel();
            timer.cancel();
        }).on_failure([&](auto&& err){
            second_failure = true;
        }).then([&](){
            finally_called = true;
        });

        handle.cancel();
        io.run();
        CHECK(finally_called);
        CHECK_FALSE(second_failure);
    }

    SECTION("Cancel, mixed")
    {
        auto test_timer = asio::steady_timer{io, 50ms};
        auto finally_called = false;
        auto second_failure = false;

        auto handle = asy::op([&](asy::context<int> ctx){
            test_timer.async_wait([ctx](const asio::error_code& err)
            {
                CHECK(err == make_error_code(asio::error::operation_aborted));
                if (!err) ctx->async_success(42);
            });
        }).then([](int&&){ FAIL("Wrong path"); }, [&](auto&& err){
            CHECK( err == std::make_error_code(std::errc::operation_canceled) );
            test_timer.cancel();
            timer.cancel();
        }).on_failure([&](auto&& err){
            second_failure = true;
        }).then([&](){
            finally_called = true;
        });

        handle.cancel();
        io.run();
        CHECK(finally_called);
        CHECK_FALSE(second_failure);
    }

    SECTION("Cancel, pending error")
    {
        auto test_timer = asio::steady_timer{io, 50ms};

        auto handle = asy::op([&](asy::context<int> ctx){
            test_timer.async_wait([ctx](const asio::error_code& err)
            {
                CHECK(err == make_error_code(asio::error::operation_aborted));
                if (!err) ctx->async_success(42);
            });
        });

        io.post([&]()
        {
            handle.cancel();
            handle.then([](auto){ FAIL("Wrong path"); }, [&](auto err){
                CHECK( err == std::make_error_code(std::errc::operation_canceled) );
                test_timer.cancel();
                timer.cancel();
            });
        });

        io.run();
    }

    SECTION("Abort")
    {
        auto test_timer = asio::steady_timer{io, 40ms};

        auto handle = asy::op([&](asy::context<int> ctx){
            test_timer.async_wait([ctx](const asio::error_code& err)
            {
                CHECK(err != make_error_code(asio::error::operation_aborted));
                if (!err) ctx->async_success(42);
            });
        }).then([](int&&){ FAIL("Wrong path"); }, [](auto&& err){
            FAIL("Wrong path");
        }).on_failure([&](auto&& err){
            FAIL("Wrong path");
        }).then([&](){
            FAIL("Wrong path");
        });

        handle.abort();
        timer.cancel();
        io.run();
    }

    SECTION("Continuation after abort")
    {
        auto test_timer = asio::steady_timer{io, 50ms};

        auto handle = asy::op([&](asy::context<int> ctx){
            test_timer.async_wait([ctx](const asio::error_code& err)
            {
                CHECK(err == make_error_code(asio::error::operation_aborted));
                if (!err) ctx->async_success(42);
            });
        });

        io.post([&]()
        {
            handle.abort();
            handle.then([](auto){ FAIL("Wrong path"); }, [&](auto err){ FAIL("Wrong path"); });

            test_timer.cancel();
            timer.cancel();
        });

        io.run();
    }

    SECTION("Double set continuation")
    {
        auto handle = asy::op<int>(42);

        handle.then([&](int&& input){
            CHECK( input == 42 );
            timer.cancel();
        });

        handle.then([](auto){ FAIL("Wrong path"); }, [&](auto err){ FAIL("Wrong path"); });

        io.run();
    }
}
