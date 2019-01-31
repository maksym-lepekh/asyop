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

    SECTION("Bad value type")
    {
        struct test
        {
            bool has_value() { return {}; }
            void value() { }
            int error() { return {}; }
        };

        STATIC_REQUIRE_FALSE(asy::detail::is_ValueOrError<test>);
    }

    SECTION("Bad error type")
    {
        struct test
        {
            bool has_value() { return {}; }
            std::string value() { return {}; }
            void error() { }
        };

        STATIC_REQUIRE_FALSE(asy::detail::is_ValueOrError<test>);
    }

    SECTION("Bad access")
    {
        struct test
        {
            bool has_value() { return {}; }
            std::string value() { error(); return {}; }

        private:
            int error() { return {}; }
        };

        STATIC_REQUIRE_FALSE(asy::detail::is_ValueOrError<test>);
    }

    SECTION("Missing methods")
    {
        struct test
        {
            bool has_value() { return {}; }
            std::string value() { return {}; }
        };

        STATIC_REQUIRE_FALSE(asy::detail::is_ValueOrError<test>);
    }
}
