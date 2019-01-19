// Copyright 2018 Maksym Lepekh
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
#include <type_traits>
#include <asy/detail/type_traits.hpp>


void void_int_fn(int) {}
void void_void_fn() {}
char char_int_fn(int) { return {}; }
char char_void_fn() { return {}; }
char char_int_double_fn(int, double) { return {}; }

TEST_CASE("Simple continuation", "[deduce]")
{
    SECTION("void(int) function")
    {
        using T = decltype(void_int_fn);
        using info = asy::detail::functor_info<T>;

        REQUIRE_FALSE( info::is_ambiguous );
        REQUIRE( info::arg_n == 1 );
        REQUIRE( std::is_same_v<info::ret_type, void> );
        REQUIRE( std::is_same_v<info::arg1_type, int> );
    }

    SECTION("void() function")
    {
        using T = decltype(void_void_fn);
        using info = asy::detail::functor_info<T>;

        REQUIRE_FALSE( info::is_ambiguous );
        REQUIRE( info::arg_n == 0 );
        REQUIRE( std::is_same_v<info::ret_type, void> );
    }

    SECTION("char(int) function")
    {
        using T = decltype(char_int_fn);
        using info = asy::detail::functor_info<T>;

        REQUIRE_FALSE( info::is_ambiguous );
        REQUIRE( info::arg_n == 1 );
        REQUIRE( std::is_same_v<info::ret_type, char> );
        REQUIRE( std::is_same_v<info::arg1_type, int> );
    }

    SECTION("char() function")
    {
        using T = decltype(char_void_fn);
        using info = asy::detail::functor_info<T>;

        REQUIRE_FALSE( info::is_ambiguous );
        REQUIRE( info::arg_n == 0 );
        REQUIRE( std::is_same_v<info::ret_type, char> );
    }

    SECTION("char(int, double) function")
    {
        using T = decltype(char_int_double_fn);
        using info = asy::detail::functor_info<T>;

        REQUIRE_FALSE( info::is_ambiguous );
        REQUIRE( info::arg_n == 2 );
        REQUIRE( std::is_same_v<info::ret_type, char> );
        REQUIRE( std::is_same_v<info::arg1_type, int> );
    }
}

TEST_CASE("Lambda continuation", "[deduce]")
{
    SECTION("void()")
    {
        auto l = [](){};
        using info = asy::detail::functor_info<decltype(l)>;

        REQUIRE_FALSE( info::is_ambiguous );
        REQUIRE( info::arg_n == 0 );
        REQUIRE( std::is_same_v<info::ret_type, void> );
    }

    SECTION("void(int)")
    {
        auto l = [](int){};
        using info = asy::detail::functor_info<decltype(l)>;

        REQUIRE_FALSE( info::is_ambiguous );
        REQUIRE( info::arg_n == 1 );
        REQUIRE( std::is_same_v<info::ret_type, void> );
        REQUIRE( std::is_same_v<info::arg1_type, int> );
    }

    SECTION("char(int)")
    {
        auto l = [](int){ return char{}; };
        using info = asy::detail::functor_info<decltype(l)>;

        REQUIRE_FALSE( info::is_ambiguous );
        REQUIRE( info::arg_n == 1 );
        REQUIRE( std::is_same_v<info::ret_type, char> );
        REQUIRE( std::is_same_v<info::arg1_type, int> );
    }

    SECTION("char()")
    {
        auto l = [](){ return char{}; };
        using info = asy::detail::functor_info<decltype(l)>;

        REQUIRE_FALSE( info::is_ambiguous );
        REQUIRE( info::arg_n == 0 );
        REQUIRE( std::is_same_v<info::ret_type, char> );
    }

    SECTION("char(int, double)")
    {
        auto l = [](int, double){ return char{}; };
        using info = asy::detail::functor_info<decltype(l)>;

        REQUIRE_FALSE( info::is_ambiguous );
        REQUIRE( info::arg_n == 2 );
        REQUIRE( std::is_same_v<info::ret_type, char> );
        REQUIRE( std::is_same_v<info::arg1_type, int> );
    }

    SECTION("generic lambda")
    {
        auto l = [](auto){ return char{}; };
        using info = asy::detail::functor_info<decltype(l)>;

        REQUIRE( info::is_ambiguous );
    }

    SECTION("overloaded struct")
    {
        struct functor
        {
            char operator()(int) { return {}; }
            char operator()(double) { return{}; }
        };
        using info = asy::detail::functor_info<functor>;

        REQUIRE( info::is_ambiguous );
    }
}


template <typename> struct single_template_t{};
template <typename, typename, typename> struct multi_template_t{};
template <typename...> struct variadic_template_t{};

TEST_CASE("Is template specialization", "[deduce]")
{
    SECTION("Single argument template")
    {
        using ret = asy::detail::is_specialization_of<single_template_t, single_template_t<int>>;
        REQUIRE( ret::value );
        REQUIRE( std::is_same_v<ret::first_arg, int> );
    }

    SECTION("Multiple argument template")
    {
        using ret = asy::detail::is_specialization_of<multi_template_t, multi_template_t<int, double, char>>;
        REQUIRE( ret::value );
        REQUIRE( std::is_same_v<ret::first_arg, int> );
    }

    SECTION("Variadic argument template")
    {
        using ret = asy::detail::is_specialization_of<variadic_template_t, variadic_template_t<int, double, char>>;
        REQUIRE( ret::value );
        REQUIRE( std::is_same_v<ret::first_arg, int> );
    }
}
