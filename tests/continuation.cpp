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

#include <asy/core/continuation.hpp>
#include <asy/common/simple_continuation.hpp>
#include <asy/common/aret_continuation.hpp>
#include <asy/common/ctx_continuation.hpp>
#include <asy/common/voe_continuation.hpp>
#include <asy/op.hpp>

#include <string>
#include <functional>
#include "voe.hpp"

using namespace asy::concept;

TEST_CASE("Continuation type", "[deduce]")
{
    SECTION("Simple continuation")
    {
        auto l = [](int&&){ return double{}; };
        STATIC_REQUIRE(satisfies<SimpleContinuation, decltype(l), int&&>);
    }

    SECTION("Simple continuation (return void)")
    {
        auto l = [](int&&){};
        STATIC_REQUIRE(satisfies<SimpleContinuation, decltype(l), int&&>);
    }

    SECTION("Async return val continuation")
    {
        auto l = [](int&&){ return asy::op(0.0); };
        STATIC_REQUIRE(satisfies<ARetContinuation, decltype(l), int&&>);
    }

    SECTION("Async continuation")
    {
        auto l = [](asy::context<double>, int&&){};
        STATIC_REQUIRE(satisfies<CtxContinuation, decltype(l), int&&>);
    }

    SECTION("Simple continuation (void input)")
    {
        auto l = [](){ return double{}; };
        STATIC_REQUIRE(satisfies<SimpleContinuation, decltype(l)>);
    }

    SECTION("Simple continuation (return void, void input)")
    {
        auto l = [](){};
        STATIC_REQUIRE(satisfies<SimpleContinuation, decltype(l)>);
    }

    SECTION("Async return val continuation (void input)")
    {
        auto l = [](){ return asy::op(0.0); };
        STATIC_REQUIRE(satisfies<ARetContinuation, decltype(l)>);
    }

    SECTION("Async continuation (void input)")
    {
        auto l = [](asy::context<double>){};
        STATIC_REQUIRE(satisfies<CtxContinuation, decltype(l)>);
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

        STATIC_REQUIRE(!asy::continuation<decltype(l3)(int&&)>::value);
        STATIC_REQUIRE(!asy::continuation<decltype(l4)(int&&)>::value);
        STATIC_REQUIRE(!asy::continuation<decltype(l6)(int&&)>::value);
        STATIC_REQUIRE(!asy::continuation<decltype(l7)(int&&)>::value);
        STATIC_REQUIRE(!asy::continuation<decltype(l8)(int&&)>::value);

        STATIC_REQUIRE(!asy::continuation<decltype(l1)()>::value);
        STATIC_REQUIRE(!asy::continuation<decltype(l2)()>::value);
        STATIC_REQUIRE(!asy::continuation<decltype(l3)()>::value);
        STATIC_REQUIRE(!asy::continuation<decltype(l5)()>::value);
        STATIC_REQUIRE(!asy::continuation<decltype(l7)()>::value);
        STATIC_REQUIRE(!asy::continuation<decltype(l8)()>::value);
    }
}

TEST_CASE("Continuation type with aux", "[deduce]")
{
    using namespace asy::detail;

    SECTION("Simple continuation")
    {
        auto l = [](int&&, std::string){ return double{}; };
        STATIC_REQUIRE(satisfies<SimpleContinuation, decltype(l), int&&, std::string>);
    }

    SECTION("Simple continuation (return void)")
    {
        auto l = [](int&&, std::string){};
        STATIC_REQUIRE(satisfies<SimpleContinuation, decltype(l), int&&, std::string>);
    }

    SECTION("Async return val continuation")
    {
        auto l = [](int&&, std::string){ return asy::op<double>(); };
        STATIC_REQUIRE(satisfies<ARetContinuation, decltype(l), int&&, std::string>);
    }

    SECTION("Async continuation")
    {
        auto l = [](asy::context<double>, int&&, std::string){};
        STATIC_REQUIRE(satisfies<CtxContinuation, decltype(l), int&&, std::string>);
    }

    SECTION("Invalid continuations")
    {
        auto l1 = [](int&&, double){ return double{}; };
        STATIC_REQUIRE(!asy::continuation<decltype(l1)(int&&)>::value);
        STATIC_REQUIRE(!asy::continuation<decltype(l1)()>::value);
        STATIC_REQUIRE(!asy::continuation<decltype(l1)(int&&, std::string)>::value);

        auto l2 = [](asy::context<double>, int&&, std::string){ return double{}; };
        STATIC_REQUIRE(!asy::continuation<decltype(l2)(int&&)>::value);
        STATIC_REQUIRE(!asy::continuation<decltype(l2)(int&&, double)>::value);
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

        STATIC_REQUIRE(satisfies<SimpleContinuation, decltype(l1), int&&>);
        STATIC_REQUIRE(std::is_same_v<asy::continuation<decltype(l1)(int&&)>::ret_type, double>);

        STATIC_REQUIRE(satisfies<ARetContinuation, decltype(l2), int&&>);
        STATIC_REQUIRE(std::is_same_v<asy::continuation<decltype(l2)(int&&)>::ret_type, double>);

        STATIC_REQUIRE(satisfies<CtxContinuation, decltype(l3), int&&>);
        STATIC_REQUIRE(std::is_same_v<asy::continuation<decltype(l3)(int&&)>::ret_type, double>);
    }

    SECTION("Returning double with aux")
    {
        auto l1 = [](int&&, std::string){ return double{}; };
        auto l2 = [](int&&, std::string){ return asy::op<double>(); };
        auto l3 = [](asy::context<double>, int&&, std::string){ };

        STATIC_REQUIRE(satisfies<SimpleContinuation, decltype(l1), int&&, std::string>);
        STATIC_REQUIRE(std::is_same_v<asy::continuation<decltype(l1)(int&&, std::string)>::ret_type, double>);

        STATIC_REQUIRE(satisfies<ARetContinuation, decltype(l2), int&&, std::string>);
        STATIC_REQUIRE(std::is_same_v<asy::continuation<decltype(l2)(int&&, std::string)>::ret_type, double>);

        STATIC_REQUIRE(satisfies<CtxContinuation, decltype(l3), int&&, std::string>);
        STATIC_REQUIRE(std::is_same_v<asy::continuation<decltype(l3)(int&&, std::string)>::ret_type, double>);
    }

    SECTION("Returning void")
    {
        auto l1 = [](int&&){ };
        auto l2 = [](int&&){ return asy::op<void>(); };
        auto l3 = [](asy::context<void>, int&&){ };

        STATIC_REQUIRE(satisfies<SimpleContinuation, decltype(l1), int&&>);
        STATIC_REQUIRE(std::is_same_v<asy::continuation<decltype(l1)(int&&)>::ret_type, void>);

        STATIC_REQUIRE(satisfies<ARetContinuation, decltype(l2), int&&>);
        STATIC_REQUIRE(std::is_same_v<asy::continuation<decltype(l2)(int&&)>::ret_type, void>);

        STATIC_REQUIRE(satisfies<CtxContinuation, decltype(l3), int&&>);
        STATIC_REQUIRE(std::is_same_v<asy::continuation<decltype(l3)(int&&)>::ret_type, void>);
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

        STATIC_REQUIRE(satisfies<SimpleContinuation, f1, int&&>);
        STATIC_REQUIRE(std::is_same_v<asy::continuation<f1(int&&)>::ret_type, double>);

        STATIC_REQUIRE(satisfies<ARetContinuation, f2, int&&>);
        STATIC_REQUIRE(std::is_same_v<asy::continuation<f2(int&&)>::ret_type, double>);

        STATIC_REQUIRE(satisfies<CtxContinuation, f3, int&&>);
        STATIC_REQUIRE(std::is_same_v<asy::continuation<f3(int&&)>::ret_type, double>);
    }

    SECTION("Returning double with aux")
    {
        using f1 = std::function<double(int&&, std::string)>;
        using f2 = std::function<asy::op_handle<double>(int&&, std::string)>;
        using f3 = std::function<void(asy::context<double>, int&&, std::string)>;

        STATIC_REQUIRE(satisfies<SimpleContinuation, f1, int&&, std::string>);
        STATIC_REQUIRE(std::is_same_v<asy::continuation<f1(int&&, std::string)>::ret_type, double>);

        STATIC_REQUIRE(satisfies<ARetContinuation, f2, int&&, std::string>);
        STATIC_REQUIRE(std::is_same_v<asy::continuation<f2(int&&, std::string)>::ret_type, double>);

        STATIC_REQUIRE(satisfies<CtxContinuation, f3, int&&, std::string>);
        STATIC_REQUIRE(std::is_same_v<asy::continuation<f3(int&&, std::string)>::ret_type, double>);
    }
}

TEST_CASE("Continuation with ValueOrError", "[deduce]")
{
    auto l = [](int&&){ return voe<std::string>{}; };
    STATIC_REQUIRE(satisfies<VoEContinuation, decltype(l), int&&>);
    STATIC_REQUIRE(!satisfies<NoEContinuation, decltype(l), int&&>);
    STATIC_REQUIRE(!satisfies<VoNContinuation, decltype(l), int&&>);

    using info = asy::continuation<decltype(l)(int&&)>;
    STATIC_REQUIRE(std::is_same_v<info::ret_type, std::string>);
    STATIC_REQUIRE(std::is_same_v<info::ret_type_orig, voe<std::string>>);
}

TEST_CASE("Continuation with NoneOrError", "[deduce]")
{
    auto l = [](int&&){ return noe{}; };
    STATIC_REQUIRE(!satisfies<VoEContinuation, decltype(l), int&&>);
    STATIC_REQUIRE(satisfies<NoEContinuation, decltype(l), int&&>);
    STATIC_REQUIRE(!satisfies<VoNContinuation, decltype(l), int&&>);

    using info = asy::continuation<decltype(l)(int&&)>;
    STATIC_REQUIRE(std::is_same_v<info::ret_type, void>);
    STATIC_REQUIRE(std::is_same_v<info::ret_type_orig, noe>);
}

TEST_CASE("Continuation with ValueOrNone", "[deduce]")
{
    auto l = [](int&&){ return von<std::string>{}; };
    STATIC_REQUIRE(!satisfies<VoEContinuation, decltype(l), int&&>);
    STATIC_REQUIRE(!satisfies<NoEContinuation, decltype(l), int&&>);
    STATIC_REQUIRE(satisfies<VoNContinuation, decltype(l), int&&>);

    using info = asy::continuation<decltype(l)(int&&)>;
    STATIC_REQUIRE(std::is_same_v<info::ret_type, std::string>);
    STATIC_REQUIRE(std::is_same_v<info::ret_type_orig, von<std::string>>);
}
