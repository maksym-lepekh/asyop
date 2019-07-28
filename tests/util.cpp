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
#include <type_traits>
#include <asy/common/util.hpp>
#include <asy/core/executor.hpp>
#include <asy/op.hpp>
#include <exception>

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
        using info = asy::util::functor<T>;

        STATIC_REQUIRE_FALSE( info::is_ambiguous );
        STATIC_REQUIRE( info::arg_n == 1 );
        STATIC_REQUIRE( std::is_same_v<info::ret_type, void> );
        STATIC_REQUIRE( std::is_same_v<info::arg1_type, int> );
    }

    SECTION("void() function")
    {
        using T = decltype(void_void_fn);
        using info = asy::util::functor<T>;

        STATIC_REQUIRE_FALSE( info::is_ambiguous );
        STATIC_REQUIRE( info::arg_n == 0 );
        STATIC_REQUIRE( std::is_same_v<info::ret_type, void> );
    }

    SECTION("char(int) function")
    {
        using T = decltype(char_int_fn);
        using info = asy::util::functor<T>;

        STATIC_REQUIRE_FALSE( info::is_ambiguous );
        STATIC_REQUIRE( info::arg_n == 1 );
        STATIC_REQUIRE( std::is_same_v<info::ret_type, char> );
        STATIC_REQUIRE( std::is_same_v<info::arg1_type, int> );
    }

    SECTION("char() function")
    {
        using T = decltype(char_void_fn);
        using info = asy::util::functor<T>;

        STATIC_REQUIRE_FALSE( info::is_ambiguous );
        STATIC_REQUIRE( info::arg_n == 0 );
        STATIC_REQUIRE( std::is_same_v<info::ret_type, char> );
    }

    SECTION("char(int, double) function")
    {
        using T = decltype(char_int_double_fn);
        using info = asy::util::functor<T>;

        STATIC_REQUIRE_FALSE( info::is_ambiguous );
        STATIC_REQUIRE( info::arg_n == 2 );
        STATIC_REQUIRE( std::is_same_v<info::ret_type, char> );
        STATIC_REQUIRE( std::is_same_v<info::arg1_type, int> );
        STATIC_REQUIRE( std::is_same_v<std::tuple_element_t<0, info::args_type>, int> );
        STATIC_REQUIRE( std::is_same_v<std::tuple_element_t<1, info::args_type>, double> );
    }
}

TEST_CASE("Lambda continuation", "[deduce]")
{
    SECTION("void()")
    {
        auto l = [](){};
        using info = asy::util::functor<decltype(l)>;

        STATIC_REQUIRE_FALSE( info::is_ambiguous );
        STATIC_REQUIRE( info::arg_n == 0 );
        STATIC_REQUIRE( std::is_same_v<info::ret_type, void> );
    }

    SECTION("void(int)")
    {
        auto l = [](int){};
        using info = asy::util::functor<decltype(l)>;

        STATIC_REQUIRE_FALSE( info::is_ambiguous );
        STATIC_REQUIRE( info::arg_n == 1 );
        STATIC_REQUIRE( std::is_same_v<info::ret_type, void> );
        STATIC_REQUIRE( std::is_same_v<info::arg1_type, int> );
    }

    SECTION("char(int)")
    {
        auto l = [](int){ return char{}; };
        using info = asy::util::functor<decltype(l)>;

        STATIC_REQUIRE_FALSE( info::is_ambiguous );
        STATIC_REQUIRE( info::arg_n == 1 );
        STATIC_REQUIRE( std::is_same_v<info::ret_type, char> );
        STATIC_REQUIRE( std::is_same_v<info::arg1_type, int> );
    }

    SECTION("char()")
    {
        auto l = [](){ return char{}; };
        using info = asy::util::functor<decltype(l)>;

        STATIC_REQUIRE_FALSE( info::is_ambiguous );
        STATIC_REQUIRE( info::arg_n == 0 );
        STATIC_REQUIRE( std::is_same_v<info::ret_type, char> );
    }

    SECTION("char(int, double)")
    {
        auto l = [](int, double){ return char{}; };
        using info = asy::util::functor<decltype(l)>;

        STATIC_REQUIRE_FALSE( info::is_ambiguous );
        STATIC_REQUIRE( info::arg_n == 2 );
        STATIC_REQUIRE( std::is_same_v<info::ret_type, char> );
        STATIC_REQUIRE( std::is_same_v<info::arg1_type, int> );
        STATIC_REQUIRE( std::is_same_v<std::tuple_element_t<0, info::args_type>, int> );
        STATIC_REQUIRE( std::is_same_v<std::tuple_element_t<1, info::args_type>, double> );
    }

    SECTION("generic lambda")
    {
        auto l = [](auto){ return char{}; };
        using info = asy::util::functor<decltype(l)>;

        STATIC_REQUIRE( info::is_ambiguous );
    }

    SECTION("overloaded struct")
    {
        struct functor
        {
            char operator()(int) { return {}; }
            char operator()(double) { return{}; }
        };
        using info = asy::util::functor<functor>;

        STATIC_REQUIRE( info::is_ambiguous );
    }
}


template <typename> struct single_template_t{};
template <typename, typename, typename> struct multi_template_t{};
template <typename...> struct variadic_template_t{};

TEST_CASE("Is template specialization", "[deduce]")
{
    SECTION("Single argument template")
    {
        using ret = asy::util::specialization_of<single_template_t, single_template_t<int>>;
        STATIC_REQUIRE( ret::value );
        STATIC_REQUIRE( std::is_same_v<ret::first_arg, int> );
    }

    SECTION("Multiple argument template")
    {
        using ret = asy::util::specialization_of<multi_template_t, multi_template_t<int, double, char>>;
        STATIC_REQUIRE( ret::value );
        STATIC_REQUIRE( std::is_same_v<ret::first_arg, int> );
    }

    SECTION("Variadic argument template")
    {
        using ret = asy::util::specialization_of<variadic_template_t, variadic_template_t<int, double, char>>;
        STATIC_REQUIRE( ret::value );
        STATIC_REQUIRE( std::is_same_v<ret::first_arg, int> );
    }
}

struct my_err
{
    my_err(std::exception_ptr ptr): e(ptr) {}
    std::exception_ptr e;
};

namespace asy::detail
{
    template <> struct error_traits<my_err>
    {
        static my_err get_cancelled()
        {
            try
            {
                throw std::logic_error("cancelled");
            }
            catch (...)
            {
                return my_err(std::current_exception());
            }
        }
    };
}

TEST_CASE("safe_invoke", "[exceptions]")
{
    asy::executor::set_impl(std::this_thread::get_id(), [](auto&& f){ f(); }, false);
    auto called = false;

    SECTION("Non-exception error type")
    {
        auto ctx = std::make_shared<asy::basic_context<int, std::error_code>>();

        SECTION("Success")
        {
            ctx->set_continuation(
                    [&](int i){ CHECK(i == 42); called = true; },
                    [](auto&&){ FAIL("Wrong path"); });

            SECTION("Non-Void return")
            {
                asy::util::safe_invoke(ctx,
                        [ctx](int i){ ctx->async_success(std::move(i)); },
                        [](){ return 42; });
            }

            SECTION("Void return")
            {
                asy::util::safe_invoke(ctx,
                        [ctx](){ ctx->async_success(42); },
                        [](){});
            }
        }

        SECTION("Failure")
        {
            ctx->set_continuation(
                    [&](int i){ FAIL("Wrong path"); },
                    [&](auto&&){ FAIL("Wrong path"); });

            try
            {
                asy::util::safe_invoke(ctx,
                        [ctx](int i){ ctx->async_success(std::move(i)); },
                        [](){ throw std::runtime_error("oops"); return 42; });
            }
            catch (std::runtime_error& e)
            {
                called = true;
            }
        }
    }

    SECTION("Exception error type")
    {
        auto ctx = std::make_shared<asy::basic_context<int, my_err>>();

        SECTION("Success")
        {
            ctx->set_continuation(
                    [&](int i){ CHECK(i == 42); called = true; },
                    [](auto&&){ FAIL("Wrong path"); });

            asy::util::safe_invoke(ctx,
                    [ctx](int i){ ctx->async_success(std::move(i)); },
                    [](){ return 42; });
        }

        SECTION("Failure")
        {
            ctx->set_continuation(
                    [&](int i){ FAIL("Wrong path"); },
                    [&](my_err&& err)
                    {
                        try
                        {
                            std::rethrow_exception(err.e);
                        }
                        catch (std::runtime_error& e)
                        {
                            called = true;
                            CHECK(e.what() == std::string("oops"));
                        }
                    });

            asy::util::safe_invoke(ctx,
                    [ctx](int i){ ctx->async_success(std::move(i)); },
                    [](){ throw std::runtime_error("oops"); return 42; });
        }
    }

    CHECK(called);
}
