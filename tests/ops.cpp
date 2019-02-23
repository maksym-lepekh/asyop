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

TEST_CASE("Compound then", "[asio]")
{
    auto io = asio::io_service{};
    auto timer = asio::steady_timer{io, 50ms};

    timer.async_wait([](const asio::error_code& err) {
        if (!err) FAIL("Timeout");
    });

    asy::this_thread::set_event_loop(io);

    SECTION("when_all: success")
    {
        auto handle = asy::when_all([]{ return 42; }, []{ return "abc"s; });

        using first_t = std::variant<std::monostate, int, std::error_code>;
        using second_t = std::variant<std::monostate, std::string, std::error_code>;
        STATIC_REQUIRE(std::is_same_v< decltype(handle), asy::op_handle<std::tuple<first_t, second_t>> >);

        handle.then([&](std::tuple<first_t, second_t>&& input)
        {
            auto& [first, second] = input;

            REQUIRE(first.index() == 1);
            CHECK(std::get<1>(first) == 42);
            REQUIRE(second.index() == 1);
            CHECK(std::get<1>(second) == "abc");

            timer.cancel();
        });

        io.run();
    }

    SECTION("when_all: failure")
    {
        auto handle = asy::when_all([](asy::context<int> ctx){ ctx->async_failure(); }, []{ return "abc"s; });

        using first_t = std::variant<std::monostate, int, std::error_code>;
        using second_t = std::variant<std::monostate, std::string, std::error_code>;
        STATIC_REQUIRE(std::is_same_v< decltype(handle), asy::op_handle<std::tuple<first_t, second_t>> >);

        handle.then([&](std::tuple<first_t, second_t>&& input)
        {
            auto& [first, second] = input;

            REQUIRE(first.index() == 2);
            CHECK(std::get<2>(first) == std::error_code{});
            REQUIRE(second.index() == 1);
            CHECK(std::get<1>(second) == "abc");

            timer.cancel();
        });

        io.run();
    }

    SECTION("when_success: success")
    {
        auto timer2 = asio::steady_timer{io, 5ms};

        auto handle = asy::when_success(
                []{ return 42; },
                [&](asy::context<std::string> ctx)
                {
                    timer2.async_wait([ctx](const asio::error_code& err) {
                        ctx->async_return("abc");
                    });
                });

        STATIC_REQUIRE(std::is_same_v< decltype(handle), asy::op_handle<std::tuple<int, std::string>> >);

        handle.then([&](std::tuple<int, std::string>&& input)
        {
            auto& [first, second] = input;
            CHECK(first == 42);
            CHECK(second == "abc");
            timer.cancel();
        });

        io.run();
    }

    SECTION("when_success: failure")
    {
        auto timer2 = asio::steady_timer{io, 5ms};

        auto handle = asy::when_success(
                []{ return 42; },
                asy::op([&](asy::context<void> ctx)
                {
                    timer2.async_wait([ctx](const asio::error_code& err) {
                        ctx->async_return();
                    });
                })
                .then([](asy::context<std::string> ctx)
                {
                    ctx->async_return("abc");
                    FAIL("Not cancelled");
                }),
                [](asy::context<int> ctx){ ctx->async_failure(); });

        STATIC_REQUIRE(std::is_same_v< decltype(handle), asy::op_handle<std::tuple<int, std::string, int>> >);

        handle.then([&](auto&& input){ FAIL("Wrong path"); })
        .on_failure([&](auto&& err)
        {
            CHECK(err == std::error_code{});
            timer.cancel();
        });

        io.run();
    }

    SECTION("when_any: success")
    {
        auto timer2 = asio::steady_timer{io, 5ms};

        auto handle = asy::when_any(
                []{ return 42; },
                asy::op([&](asy::context<void> ctx)
                {
                    timer2.async_wait([ctx](const asio::error_code& err) {
                        ctx->async_return();
                    });
                })
                .then([](asy::context<std::string> ctx)
                {
                    ctx->async_return("abc");
                    FAIL("Not cancelled");
                }));

        STATIC_REQUIRE(std::is_same_v< decltype(handle), asy::op_handle<std::variant<int, std::string>> >);

        handle.then([&](std::variant<int, std::string>&& input)
        {
            REQUIRE(input.index() == 0);
            CHECK(std::get<0>(input) == 42);
            timer.cancel();
        });

        io.run();
    }

    SECTION("when_any: failure")
    {
        auto timer2 = asio::steady_timer{io, 5ms};

        auto handle = asy::when_any(
                [](asy::context<int> ctx){ ctx->async_failure(); },
                asy::op([&](asy::context<void> ctx)
                {
                    timer2.async_wait([ctx](const asio::error_code& err) {
                        ctx->async_return();
                    });
                })
                .then([](asy::context<std::string> ctx)
                {
                    ctx->async_return("abc");
                    FAIL("Not cancelled");
                }));

        STATIC_REQUIRE(std::is_same_v< decltype(handle), asy::op_handle<std::variant<int, std::string>> >);

        handle.then([&](auto&& input){ FAIL("Wrong path"); })
        .on_failure([&](auto&& err)
        {
            CHECK(err == std::error_code{});
            timer.cancel();
        });

        io.run();
    }

    SECTION("when_all: all failure")
    {
        auto timer2 = asio::steady_timer{io, 5ms};
        auto timer3 = asio::steady_timer{io, 10ms};

        auto handle = asy::when_all(
                [&](asy::context<int> ctx)
                {
                    timer3.async_wait([ctx](const asio::error_code& err) {
                        ctx->async_failure(std::make_error_code(std::errc::bad_address));
                    });
                },
                asy::op([&](asy::context<void> ctx)
                {
                    timer2.async_wait([ctx](const asio::error_code& err) {
                        ctx->async_return();
                    });
                })
                .then([](asy::context<std::string> ctx)
                {
                    ctx->async_failure(std::make_error_code(std::errc::operation_not_permitted));
                }));

        using first_t = std::variant<std::monostate, int, std::error_code>;
        using second_t = std::variant<std::monostate, std::string, std::error_code>;
        STATIC_REQUIRE(std::is_same_v< decltype(handle), asy::op_handle<std::tuple<first_t, second_t>> >);

        handle.then([&](std::tuple<first_t, second_t>&& input)
                    {
                        auto& [first, second] = input;

                        REQUIRE(first.index() == 2);
                        CHECK(std::get<2>(first) == std::make_error_code(std::errc::bad_address));
                        REQUIRE(second.index() == 2);
                        CHECK(std::get<2>(second) == std::make_error_code(std::errc::operation_not_permitted));

                        timer.cancel();
                    });

        io.run();
    }

    SECTION("when_all: cancel")
    {
        auto timer2 = asio::steady_timer{io, 10ms};
        auto timer3 = asio::steady_timer{io, 15ms};

        auto handle = asy::when_all(
                asy::op([&](asy::context<void> ctx)
                {
                    timer2.async_wait([ctx](const asio::error_code& err) {
                        ctx->async_return();
                    });
                })
                .then([](asy::context<int> ctx)
                {
                    FAIL("Not cancelled");
                    ctx->async_return(42);
                }),
                asy::op([&](asy::context<void> ctx)
                {
                    timer3.async_wait([ctx](const asio::error_code& err) {
                        ctx->async_return();
                    });
                })
                .then([](asy::context<std::string> ctx)
                {
                    FAIL("Not cancelled");
                    ctx->async_return("abc");
                }));

        using first_t = std::variant<std::monostate, int, std::error_code>;
        using second_t = std::variant<std::monostate, std::string, std::error_code>;
        STATIC_REQUIRE(std::is_same_v< decltype(handle), asy::op_handle<std::tuple<first_t, second_t>> >);

        handle.then([](std::tuple<first_t, second_t>&& input)
        {
            FAIL("Not cancelled");
        },
        [&](auto&& err)
        {
            CHECK(err == std::make_error_code(std::errc::operation_canceled));
            timer.cancel();
        });

        auto cancel_timer = asio::steady_timer{io, 1ms};
        cancel_timer.async_wait([&](const asio::error_code& err) {
            handle.cancel();
        });

        io.run();
    }

    SECTION("when_success: cancel")
    {
        auto timer2 = asio::steady_timer{io, 5ms};
        auto timer3 = asio::steady_timer{io, 10ms};

        auto handle = asy::when_success(
                asy::op([&](asy::context<void> ctx)
                {
                    timer2.async_wait([ctx](const asio::error_code& err) {
                        ctx->async_return();
                    });
                })
                .then([](asy::context<int> ctx)
                {
                    FAIL("Not cancelled");
                    ctx->async_return(42);
                }),
                asy::op([&](asy::context<void> ctx)
                {
                    timer3.async_wait([ctx](const asio::error_code& err) {
                        ctx->async_return();
                    });
                })
                .then([](asy::context<std::string> ctx)
                {
                    FAIL("Not cancelled");
                    ctx->async_return("abc");
                }));

        STATIC_REQUIRE(std::is_same_v< decltype(handle), asy::op_handle<std::tuple<int, std::string>> >);

        handle.then([](std::tuple<int, std::string>&& input)
        {
            FAIL("Not cancelled");
        },
        [&](auto&& err)
        {
            CHECK(err == std::make_error_code(std::errc::operation_canceled));
            timer.cancel();
        });

        auto cancel_timer = asio::steady_timer{io, 1ms};
        cancel_timer.async_wait([&](const asio::error_code& err) {
            handle.cancel();
        });

        io.run();
    }

    SECTION("when_any: cancel")
    {
        auto timer2 = asio::steady_timer{io, 5ms};
        auto timer3 = asio::steady_timer{io, 10ms};

        auto handle = asy::when_any(
                asy::op([&](asy::context<void> ctx)
                {
                    timer2.async_wait([ctx](const asio::error_code& err) {
                        ctx->async_return();
                    });
                })
                .then([](asy::context<int> ctx)
                {
                    FAIL("Not cancelled");
                    ctx->async_return(42);
                }),
                asy::op([&](asy::context<void> ctx)
                {
                    timer3.async_wait([ctx](const asio::error_code& err) {
                        ctx->async_return();
                    });
                })
                .then([](asy::context<std::string> ctx)
                {
                    FAIL("Not cancelled");
                    ctx->async_return("abc");
                }));

        STATIC_REQUIRE(std::is_same_v< decltype(handle), asy::op_handle<std::variant<int, std::string>> >);

        handle.then([](std::variant<int, std::string>&& input)
        {
            FAIL("Not cancelled");
        },
        [&](auto&& err)
        {
            CHECK(err == std::make_error_code(std::errc::operation_canceled));
            timer.cancel();
        });

        auto cancel_timer = asio::steady_timer{io, 1ms};
        cancel_timer.async_wait([&](const asio::error_code& err) {
            handle.cancel();
        });

        io.run();
    }
}
