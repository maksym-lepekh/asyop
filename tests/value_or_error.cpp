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
#include <asy/detail/value_or_error.hpp>
#include <string>
#include "voe.hpp"

TEST_CASE("ValueOrError", "[deduce]")
{
    SECTION("Proper type")
    {
        struct test
        {
            bool has_value() { return {}; }
            std::string value() { return {}; }
            int error() { return {}; }
        };

        using info = asy::detail::ValueOrError<test>;
        STATIC_REQUIRE(info::value);
        STATIC_REQUIRE(std::is_same_v<info::success_type, std::string>);
        STATIC_REQUIRE(std::is_same_v<info::failure_type, int>);
    }

    SECTION("Bad has_value type")
    {
        struct test
        {
            std::string has_value() { return {}; }
            std::string value() { return {}; }
            int error() { return {}; }
        };

        STATIC_REQUIRE_FALSE(asy::detail::is_ValueOrError<test>);
    }

    SECTION("Void value type")
    {
        struct test
        {
            bool has_value() { return {}; }
            void value() { }
            int error() { return {}; }
        };

        STATIC_REQUIRE(asy::detail::is_ValueOrError<test>);
        STATIC_REQUIRE(asy::detail::ValueOrError<test>::voe_type == asy::detail::voe_t::noe);
    }

    SECTION("Bad error type")
    {
        struct test
        {
            bool has_value() { return {}; }
            std::string value() { return {}; }
            void error() { }
        };

        STATIC_REQUIRE(asy::detail::is_ValueOrError<test>);
        STATIC_REQUIRE(asy::detail::ValueOrError<test>::voe_type == asy::detail::voe_t::von);
    }

    SECTION("Bad access")
    {
        struct test
        {
        private:
            bool has_value() { return {}; }
            std::string value() { error(); return {}; }
            int error() { return {}; }
        };

        STATIC_REQUIRE_FALSE(asy::detail::is_ValueOrError<test>);
    }

    SECTION("Missing methods")
    {
        struct test
        {
            bool has_value() { return {}; }
            std::string get_value() { return {}; }
        };

        STATIC_REQUIRE_FALSE(asy::detail::is_ValueOrError<test>);
    }
}

TEST_CASE("Testing VoE, VoN, NoE types", "[deduce]")
{
    SECTION("VoE")
    {
        using info = asy::detail::ValueOrError<voe<std::string>>;
        STATIC_REQUIRE(info::value);
        STATIC_REQUIRE(info::voe_type == asy::detail::voe_t::voe);
        STATIC_REQUIRE(std::is_same_v<info::success_type, std::string>);
        STATIC_REQUIRE(std::is_same_v<info::failure_type, std::error_code>);
    }

    SECTION("VoN")
    {
        using info = asy::detail::ValueOrError<von<std::string>>;
        STATIC_REQUIRE(info::value);
        STATIC_REQUIRE(info::voe_type == asy::detail::voe_t::von);
        STATIC_REQUIRE(std::is_same_v<info::success_type, std::string>);
        STATIC_REQUIRE(std::is_same_v<info::failure_type, void>);
    }

    SECTION("NoE")
    {
        using info = asy::detail::ValueOrError<noe>;
        STATIC_REQUIRE(info::value);
        STATIC_REQUIRE(info::voe_type == asy::detail::voe_t::noe);
        STATIC_REQUIRE(std::is_same_v<info::success_type, void>);
        STATIC_REQUIRE(std::is_same_v<info::failure_type, std::error_code>);
    }
}
