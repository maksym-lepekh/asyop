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
#pragma once

#include <type_traits>
#include <tuple>
#include <memory>


namespace asy::detail
{
    template <template <typename...> typename, typename...>
    struct specialization_of : std::false_type {};

    template <template <typename...> typename Templ, typename... OtherArgs, typename TemplArg>
    struct specialization_of<Templ, Templ<TemplArg, OtherArgs...>> : std::true_type {
       using first_arg = TemplArg;
    };


    template <typename F>
    struct functor_info_fn
    {
        static constexpr auto is_ambiguous = true;
    };

    template <typename Ret>
    struct functor_info_fn<Ret()>
    {
        static constexpr auto is_ambiguous = false;
        static constexpr auto arg_n = std::size_t{};
        using ret_type = Ret;
    };

    template <typename Ret, typename Arg1, typename... Args>
    struct functor_info_fn<Ret(Arg1, Args...)>
    {
        static constexpr auto is_ambiguous = false;
        static constexpr auto arg_n = sizeof...(Args) + 1;
        using ret_type = Ret;
        using arg1_type = Arg1;
        using args_type = std::tuple<Arg1, Args...>;
    };

    template <typename Ret, typename... Args>
    struct functor_info_fn<Ret(Args...) const>: functor_info_fn<Ret(Args...)> {};

    template <typename Ret, typename... Args>
    struct functor_info_fn<Ret(Args...) volatile>: functor_info_fn<Ret(Args...)> {};

    template <typename Ret, typename... Args>
    struct functor_info_fn<Ret(Args...) const volatile>: functor_info_fn<Ret(Args...)> {};


    template <typename F>
    struct functor_info_mem
    {
        static constexpr auto is_ambiguous = true;
    };

    template <typename F, typename Fn>
    struct functor_info_mem<Fn F::*>: public functor_info_fn<Fn>{};

    template <typename T>
    class has_call_op
    {
        template <typename C>
        static constexpr bool has(decltype(&C::operator())) { return true; }

        template <typename>
        static constexpr bool has(...) { return false; }

    public:
        static constexpr auto value = has<T>(nullptr);
    };

    template <typename F, bool>
    struct functor_info_sel;

    template <typename F>
    struct functor_info_sel<F, false>: public functor_info_fn<F>{};

    template <typename F>
    struct functor_info_sel<F, true>: public functor_info_mem<decltype(&std::remove_reference_t<F>::operator())>{};

    template <typename F>
    struct functor_info: public functor_info_sel<F, has_call_op<std::remove_reference_t<F>>::value>{};

    template <typename F, typename Arg>
    struct is_appliable: std::false_type{};

    template <typename F, typename... Args>
    struct is_appliable<F, std::tuple<Args...>>: std::is_invocable<F, Args...>{};

    template <typename F, typename ArgsTuple>
    inline constexpr auto is_appliable_v = is_appliable<F, ArgsTuple>::value;

    template <typename F, typename Arg>
    struct apply_result: std::false_type{};

    template <typename F, typename... Args>
    struct apply_result<F, std::tuple<Args...>>: std::invoke_result<F, Args...>{};

    template <typename F, typename ArgsTuple>
    using apply_result_t = typename apply_result<F, ArgsTuple>::type;

    template <typename T, typename Tuple>
    using tuple_prepend_t = decltype(std::tuple_cat(std::declval<std::tuple<T>>(), std::declval<Tuple>()));

    template <template <typename...> typename Templ, typename... OtherArgs>
    using specialization_of_first_t = typename detail::specialization_of<Templ, OtherArgs...>::first_arg;

    template <typename F>
    using functor_ret_t = typename detail::functor_info<F>::ret_type;

    template <typename F>
    using functor_first_t = typename detail::functor_info<F>::arg1_type;
}
