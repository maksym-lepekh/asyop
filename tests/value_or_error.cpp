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
#include <asy/common/voe_continuation.hpp>
#include <string>
#include "voe.hpp"

using namespace asy;

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

        STATIC_REQUIRE(concept::satisfies<concept::ValueOrError, test>);
        STATIC_REQUIRE_FALSE(concept::satisfies<concept::ValueOrNone, test>);
        STATIC_REQUIRE_FALSE(concept::satisfies<concept::NoneOrError, test>);
    }

    SECTION("Bad has_value type")
    {
        struct test
        {
            std::string has_value() { return {}; }
            std::string value() { return {}; }
            int error() { return {}; }
        };

        STATIC_REQUIRE_FALSE(concept::satisfies<concept::ValueOrError, test>);
        STATIC_REQUIRE_FALSE(concept::satisfies<concept::ValueOrNone, test>);
        STATIC_REQUIRE_FALSE(concept::satisfies<concept::NoneOrError, test>);
    }

    SECTION("Void value type")
    {
        struct test
        {
            bool has_value() { return {}; }
            void value() { }
            int error() { return {}; }
        };

        STATIC_REQUIRE_FALSE(concept::satisfies<concept::ValueOrError, test>);
        STATIC_REQUIRE_FALSE(concept::satisfies<concept::ValueOrNone, test>);
        STATIC_REQUIRE(concept::satisfies<concept::NoneOrError, test>);
    }

    SECTION("Bad error type")
    {
        struct test
        {
            bool has_value() { return {}; }
            std::string value() { return {}; }
            void error() { }
        };

        STATIC_REQUIRE_FALSE(concept::satisfies<concept::ValueOrError, test>);
        STATIC_REQUIRE(concept::satisfies<concept::ValueOrNone, test>);
        STATIC_REQUIRE_FALSE(concept::satisfies<concept::NoneOrError, test>);
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

        STATIC_REQUIRE_FALSE(concept::satisfies<concept::ValueOrError, test>);
        STATIC_REQUIRE_FALSE(concept::satisfies<concept::ValueOrNone, test>);
        STATIC_REQUIRE_FALSE(concept::satisfies<concept::NoneOrError, test>);
    }

    SECTION("Missing methods")
    {
        struct test
        {
            bool has_value() { return {}; }
            std::string get_value() { return {}; }
        };

        STATIC_REQUIRE_FALSE(concept::satisfies<concept::ValueOrError, test>);
        STATIC_REQUIRE_FALSE(concept::satisfies<concept::ValueOrNone, test>);
        STATIC_REQUIRE_FALSE(concept::satisfies<concept::NoneOrError, test>);
    }
}

TEST_CASE("VoE, VoN, NoE types", "[deduce]")
{
    SECTION("VoE")
    {
        using my_t = voe<std::string>;
        STATIC_REQUIRE(concept::satisfies<concept::ValueOrError, my_t>);
        STATIC_REQUIRE_FALSE(concept::satisfies<concept::ValueOrNone, my_t>);
        STATIC_REQUIRE_FALSE(concept::satisfies<concept::NoneOrError, my_t>);
    }

    SECTION("VoN")
    {
        using my_t = von<std::string>;
        STATIC_REQUIRE_FALSE(concept::satisfies<concept::ValueOrError, my_t>);
        STATIC_REQUIRE(concept::satisfies<concept::ValueOrNone, my_t>);
        STATIC_REQUIRE_FALSE(concept::satisfies<concept::NoneOrError, my_t>);
    }

    SECTION("NoE")
    {
        using my_t = noe;
        STATIC_REQUIRE_FALSE(concept::satisfies<concept::ValueOrError, my_t>);
        STATIC_REQUIRE_FALSE(concept::satisfies<concept::ValueOrNone, my_t>);
        STATIC_REQUIRE(concept::satisfies<concept::NoneOrError, my_t>);
    }
}
