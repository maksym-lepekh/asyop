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

#include <asy/op.hpp>
#include <asy/evloop_asio.hpp>

#include <vector>
#include <list>
#include <set>
#include <unordered_set>
#include <array>
#include <sstream>
#include <string>


TEST_CASE("op_handle compliance")
{
    auto io = asio::io_service{};
    asy::this_thread::set_event_loop(io);

    SECTION("Compare")
    {
        auto op1 = asy::op<int>(42);
        auto op2 = asy::op<int>(43);

        auto l = op1 < op2;
        auto g = op1 > op2;
        auto le = op1 <= op2;
        auto ge = op1 >= op2;
        auto e = op1 == op2;
        auto ne = op1 != op2;

        REQUIRE(ne);
        REQUIRE_FALSE(e);
        REQUIRE(l != ge);
        REQUIRE(g != le);
    }

    SECTION("Containers")
    {
        auto op1 = asy::op<int>(42);
        auto op2 = asy::op<int>(43);
        auto op3 = asy::op<int>(44);

        [[maybe_unused]] auto dummy_v = std::vector{op1, op2, op3};
        [[maybe_unused]] auto dummy_l = std::list{op1, op2, op3};
        [[maybe_unused]] auto dummy_a = std::array{op1, op2, op3};

        auto dummy_s = std::set{op1, op2, op3};
        auto dummy_us = std::unordered_set<decltype(op1)>{op1, op2, op3};

        REQUIRE(dummy_s.size() == 3);
        REQUIRE(dummy_us.size() == 3);
    }

    SECTION("Output")
    {
        using Catch::Matchers::Matches;

        auto op = asy::op<int>(42);

        auto ss = std::stringstream{};
        ss << op << std::endl;
        auto stream_res = ss.str();

        auto to_string_res = to_string(op);

        REQUIRE_THAT(stream_res, Matches("^op_handle\\{\\w+\\}\\n"));
        REQUIRE_THAT(to_string_res, Matches("^op_handle\\{\\w+\\}"));
    }

    SECTION("Swap")
    {
        auto op1 = asy::op<int>(42);
        auto op2 = asy::op<int>(43);

        auto l_before = op1 < op2;
        swap(op1, op2);
        auto l_after = op1 < op2;

        auto op1_res = int{};
        auto op2_res = int{};

        op1.then([&](int i){ op1_res = i; });
        op2.then([&](int i){ op2_res = i; });

        io.run();

        REQUIRE(l_before != l_after);
        REQUIRE(op1_res == 43);
        REQUIRE(op2_res == 42);
    }
}
