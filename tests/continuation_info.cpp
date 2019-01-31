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
#include <string>
#include <functional>
#include <asy/detail/continuation_info.hpp>
#include <asy/op.hpp>

TEST_CASE("Continuation type", "[deduce]")
{
    using namespace asy::detail;

    SECTION("Simple continuation")
    {
        auto l = [](int&&){ return double{}; };
        constexpr auto l_type = get_cont_type<decltype(l), int>();
        STATIC_REQUIRE(l_type == cont_type::simple);
    }

    SECTION("Simple continuation (return void)")
    {
        auto l = [](int&&){};
        constexpr auto l_type = get_cont_type<decltype(l), int>();
        STATIC_REQUIRE(l_type == cont_type::simple);
    }

    SECTION("Async return val continuation")
    {
        auto l = [](int&&){ return asy::op(0.0); };
        constexpr auto l_type = get_cont_type<decltype(l), int>();
        STATIC_REQUIRE(l_type == cont_type::areturn);
    }

    SECTION("Async continuation")
    {
        auto l = [](asy::context<double>, int&&){};
        constexpr auto l_type = get_cont_type<decltype(l), int>();
        STATIC_REQUIRE(l_type == cont_type::async);
    }

    SECTION("Simple continuation (void input)")
    {
        auto l = [](){ return double{}; };
        constexpr auto l_type = get_cont_type<decltype(l), void>();
        STATIC_REQUIRE(l_type == cont_type::simple);
    }

    SECTION("Simple continuation (return void, void input)")
    {
        auto l = [](){};
        constexpr auto l_type = get_cont_type<decltype(l), void>();
        STATIC_REQUIRE(l_type == cont_type::simple);
    }

    SECTION("Async return val continuation (void input)")
    {
        auto l = [](){ return asy::op(0.0); };
        constexpr auto l_type = get_cont_type<decltype(l), void>();
        STATIC_REQUIRE(l_type == cont_type::areturn);
    }

    SECTION("Async continuation (void input)")
    {
        auto l = [](asy::context<double>){};
        constexpr auto l_type = get_cont_type<decltype(l), void>();
        STATIC_REQUIRE(l_type == cont_type::async);
    }

    SECTION("Invalid continuations")
    {
        auto l1 = [](int){ return double{}; };
        auto l2 = [](int&&, double){ return double{}; };
        auto l3 = [](std::string){ return double{}; };
        auto l4 = [](){};
        auto l5 = [](asy::context<double>, int){ };
        auto l6 = [](asy::context<double>){ };
        auto l7 = [](asy::context<double>, int&&){ return double{}; };
        auto l8 = [](asy::context<double>, int&&){ return asy::op<double>(0.0); };

        STATIC_REQUIRE(get_cont_type<decltype(l2), int>() == cont_type::invalid);
        STATIC_REQUIRE(get_cont_type<decltype(l3), int>() == cont_type::invalid);
        STATIC_REQUIRE(get_cont_type<decltype(l4), int>() == cont_type::invalid);
        STATIC_REQUIRE(get_cont_type<decltype(l6), int>() == cont_type::invalid);
        STATIC_REQUIRE(get_cont_type<decltype(l7), int>() == cont_type::invalid);
        STATIC_REQUIRE(get_cont_type<decltype(l8), int>() == cont_type::invalid);

        STATIC_REQUIRE(get_cont_type<decltype(l1), void>() == cont_type::invalid);
        STATIC_REQUIRE(get_cont_type<decltype(l2), void>() == cont_type::invalid);
        STATIC_REQUIRE(get_cont_type<decltype(l3), void>() == cont_type::invalid);
        STATIC_REQUIRE(get_cont_type<decltype(l5), void>() == cont_type::invalid);
        STATIC_REQUIRE(get_cont_type<decltype(l7), void>() == cont_type::invalid);
        STATIC_REQUIRE(get_cont_type<decltype(l8), void>() == cont_type::invalid);
    }
}

TEST_CASE("Continuation type with aux", "[deduce]")
{
    using namespace asy::detail;

    SECTION("Simple continuation")
    {
        auto l = [](int&&, std::string){ return double{}; };
        constexpr auto l_type = get_cont_type<decltype(l), int, std::string>();
        STATIC_REQUIRE(l_type == cont_type::simple);
    }

    SECTION("Simple continuation (return void)")
    {
        auto l = [](int&&, std::string){};
        constexpr auto l_type = get_cont_type<decltype(l), int, std::string>();
        STATIC_REQUIRE(l_type == cont_type::simple);
    }

    SECTION("Async return val continuation")
    {
        auto l = [](int&&, std::string){ return asy::op<double>(); };
        constexpr auto l_type = get_cont_type<decltype(l), int, std::string>();
        STATIC_REQUIRE(l_type == cont_type::areturn);
    }

    SECTION("Async continuation")
    {
        auto l = [](asy::context<double>, int&&, std::string){};
        constexpr auto l_type = get_cont_type<decltype(l), int, std::string>();
        STATIC_REQUIRE(l_type == cont_type::async);
    }

    SECTION("Simple continuation (void input)")
    {
        auto l = [](std::string){ return double{}; };
        constexpr auto l_type = get_cont_type<decltype(l), void, std::string>();
        STATIC_REQUIRE(l_type == cont_type::simple);
    }

    SECTION("Simple continuation (return void, void input)")
    {
        auto l = [](std::string){};
        constexpr auto l_type = get_cont_type<decltype(l), void, std::string>();
        STATIC_REQUIRE(l_type == cont_type::simple);
    }

    SECTION("Async return val continuation (void input)")
    {
        auto l = [](std::string){ return asy::op<double>(); };
        constexpr auto l_type = get_cont_type<decltype(l), void, std::string>();
        STATIC_REQUIRE(l_type == cont_type::areturn);
    }

    SECTION("Async continuation (void input)")
    {
        auto l = [](asy::context<double>, std::string){};
        constexpr auto l_type = get_cont_type<decltype(l), void, std::string>();
        STATIC_REQUIRE(l_type == cont_type::async);
    }

    SECTION("Invalid continuations")
    {
        auto l1 = [](int&&, double){ return double{}; };
        auto l2 = [](asy::context<double>, int&&, std::string){ return double{}; };

        STATIC_REQUIRE(get_cont_type<decltype(l1), int>() == cont_type::invalid);
        STATIC_REQUIRE(get_cont_type<decltype(l1), void>() == cont_type::invalid);
        STATIC_REQUIRE(get_cont_type<decltype(l1), int, std::string>() == cont_type::invalid);

        STATIC_REQUIRE(get_cont_type<decltype(l2), int>() == cont_type::invalid);
        STATIC_REQUIRE(get_cont_type<decltype(l2), int, double>() == cont_type::invalid);
    }
}

TEST_CASE("Continuation info", "[deduce]")
{
    using namespace asy::detail;

    SECTION("Returning double")
    {
        auto l1 = [](int&&){ return double{}; };
        auto l2 = [](int&&){ return asy::op<double>(0.0); };
        auto l3 = [](asy::context<double>, int&&){ };

        STATIC_REQUIRE(continuation_info<decltype(l1), int>::type == cont_type::simple);
        STATIC_REQUIRE(std::is_same_v<continuation_info<decltype(l1), int>::ret_type, double>);

        STATIC_REQUIRE(continuation_info<decltype(l2), int>::type == cont_type::areturn);
        STATIC_REQUIRE(std::is_same_v<continuation_info<decltype(l2), int>::ret_type, double>);

        STATIC_REQUIRE(continuation_info<decltype(l3), int>::type == cont_type::async);
        STATIC_REQUIRE(std::is_same_v<continuation_info<decltype(l3), int>::ret_type, double>);
    }

    SECTION("Returning double with aux")
    {
        auto l1 = [](int&&, std::string){ return double{}; };
        auto l2 = [](int&&, std::string){ return asy::op<double>(); };
        auto l3 = [](asy::context<double>, int&&, std::string){ };

        STATIC_REQUIRE(continuation_info<decltype(l1), int, std::string>::type == cont_type::simple);
        STATIC_REQUIRE(std::is_same_v<continuation_info<decltype(l1), int, std::string>::ret_type, double>);

        STATIC_REQUIRE(continuation_info<decltype(l2), int, std::string>::type == cont_type::areturn);
        STATIC_REQUIRE(std::is_same_v<continuation_info<decltype(l2), int, std::string>::ret_type, double>);

        STATIC_REQUIRE(continuation_info<decltype(l3), int, std::string>::type == cont_type::async);
        STATIC_REQUIRE(std::is_same_v<continuation_info<decltype(l3), int, std::string>::ret_type, double>);
    }

    SECTION("Returning void")
    {
        auto l1 = [](int&&){ };
        auto l2 = [](int&&){ return asy::op<void>(); };
        auto l3 = [](asy::context<void>, int&&){ };

        STATIC_REQUIRE(continuation_info<decltype(l1), int>::type == cont_type::simple);
        STATIC_REQUIRE(std::is_same_v<continuation_info<decltype(l1), int>::ret_type, void>);

        STATIC_REQUIRE(continuation_info<decltype(l2), int>::type == cont_type::areturn);
        STATIC_REQUIRE(std::is_same_v<continuation_info<decltype(l2), int>::ret_type, void>);

        STATIC_REQUIRE(continuation_info<decltype(l3), int>::type == cont_type::async);
        STATIC_REQUIRE(std::is_same_v<continuation_info<decltype(l3), int>::ret_type, void>);
    }

    SECTION("Invalid")
    {
        auto l1 = [](int&&, double){ return double{}; };
        auto l2 = [](std::string){ return double{}; };
        auto l3 = [](){};
        auto l4 = [](asy::context<double>){ };
        auto l5 = [](asy::context<double>, int&&){ return double{}; };
        auto l6 = [](asy::context<double>, int&&){ return asy::op<double>(); };

        STATIC_REQUIRE(continuation_info<decltype(l1), int>::type == cont_type::invalid);
        STATIC_REQUIRE(continuation_info<decltype(l2), int>::type == cont_type::invalid);
        STATIC_REQUIRE(continuation_info<decltype(l3), int>::type == cont_type::invalid);
        STATIC_REQUIRE(continuation_info<decltype(l4), int>::type == cont_type::invalid);
        STATIC_REQUIRE(continuation_info<decltype(l5), int>::type == cont_type::invalid);
        STATIC_REQUIRE(continuation_info<decltype(l6), int>::type == cont_type::invalid);
    }
}

TEST_CASE("Continuation info from std::function", "[deduce]")
{
    using namespace asy::detail;

    SECTION("Returning double")
    {
        using f1 = std::function<double(int&&)>;
        using f2 = std::function<asy::op_handle<double>(int&&)>;
        using f3 = std::function<void(asy::context<double>, int&&)>;

        STATIC_REQUIRE(continuation_info<f1, int>::type == cont_type::simple);
        STATIC_REQUIRE(std::is_same_v<continuation_info<f1, int>::ret_type, double>);

        STATIC_REQUIRE(continuation_info<f2, int>::type == cont_type::areturn);
        STATIC_REQUIRE(std::is_same_v<continuation_info<f2, int>::ret_type, double>);

        STATIC_REQUIRE(continuation_info<f3, int>::type == cont_type::async);
        STATIC_REQUIRE(std::is_same_v<continuation_info<f3, int>::ret_type, double>);
    }

    SECTION("Returning double with aux")
    {
        using f1 = std::function<double(int&&, std::string)>;
        using f2 = std::function<asy::op_handle<double>(int&&, std::string)>;
        using f3 = std::function<void(asy::context<double>, int&&, std::string)>;

        STATIC_REQUIRE(continuation_info<f1, int, std::string>::type == cont_type::simple);
        STATIC_REQUIRE(std::is_same_v<continuation_info<f1, int, std::string>::ret_type, double>);

        STATIC_REQUIRE(continuation_info<f2, int, std::string>::type == cont_type::areturn);
        STATIC_REQUIRE(std::is_same_v<continuation_info<f2, int, std::string>::ret_type, double>);

        STATIC_REQUIRE(continuation_info<f3, int, std::string>::type == cont_type::async);
        STATIC_REQUIRE(std::is_same_v<continuation_info<f3, int, std::string>::ret_type, double>);
    }
}
