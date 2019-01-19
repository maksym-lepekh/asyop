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
}
