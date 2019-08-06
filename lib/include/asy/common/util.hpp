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
#include "../core/basic_context.hpp"


#if defined(__cpp_exceptions)
#define ASYOP_TRY try
#define ASYOP_CATCH catch(...)
#else
#define ASYOP_TRY if constexpr (true)
#define ASYOP_CATCH else
#endif


namespace asy::detail
{
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
        static constexpr bool has(decltype(&C::operator()) /*dummy*/) { return true; }

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
}

namespace asy::util
{
    /// Helper that provides certain information that is deduced from the functor type
    template <typename F>
    struct functor:
            public asy::detail::functor_info_sel<F, asy::detail::has_call_op<std::remove_reference_t<F>>::value>{};

    /// Deduced return type of the functor
    template <typename F>
    using functor_ret_t = typename functor<F>::ret_type;

    /// Deduced first arg of the functor
    template <typename F>
    using functor_first_t = typename functor<F>::arg1_type;

    /// A helper that represents a true type if the second arg is a specialization of the first one
    template <template <typename...> typename, typename...>
    struct specialization_of : std::false_type {};

    /// A helper that represents a true type if the second arg is a specialization of the first one
    /// Provides a type alias for the first template argument of the specialization
    template <template <typename...> typename Templ, typename TemplArg>
    struct specialization_of<Templ, Templ<TemplArg>> : std::true_type {
        using first_arg = TemplArg;
    };

    /// A helper that represents a true type if the second arg is a specialization of the first one
    /// Provides a type alias for the first, second and other template arguments of the specialization
    template <template <typename...> typename Templ, typename... OtherArgs, typename TemplArg1, typename TemplArg2>
    struct specialization_of<Templ, Templ<TemplArg1, TemplArg2, OtherArgs...>> : std::true_type {
        using first_arg = TemplArg1;
        using second_arg = TemplArg2;
        using args = std::tuple<OtherArgs...>;
    };

    /// First template argument of the tempalte specialization
    template <template <typename...> typename Templ, typename... OtherArgs>
    using specialization_of_first_t = typename specialization_of<Templ, OtherArgs...>::first_arg;

    template <template <typename...> typename Templ, typename... OtherArgs>
    using specialization_of_second_t = typename specialization_of<Templ, OtherArgs...>::second_arg;

    template <typename Err, typename F, typename... Args>
    constexpr auto should_catch = std::is_convertible_v<std::exception_ptr, Err>
            && !std::is_nothrow_invocable_v<F, Args...>;

    template <typename T, typename Err, typename Cb, typename F, typename... Args>
    auto safe_invoke(asy::basic_context_ptr<T, Err> ctx, Cb&& cb, F&& f, Args&&... args)
    {
        if constexpr (should_catch<Err, F, Args...>)
        {
            ASYOP_TRY
            {
                if constexpr (std::is_void_v<std::invoke_result_t<F, Args...>>)
                {
                    std::forward<F>(f)(std::forward<Args>(args)...);
                    std::forward<Cb>(cb)();
                }
                else
                {
                    std::forward<Cb>(cb)(std::forward<F>(f)(std::forward<Args>(args)...));
                }
            }
            ASYOP_CATCH
            {
                ctx->async_failure(std::current_exception());
            }
        }
        else
        {
            if constexpr (std::is_void_v<std::invoke_result_t<F, Args...>>)
            {
                std::forward<F>(f)(std::forward<Args>(args)...);
                std::forward<Cb>(cb)();
            }
            else
            {
                std::forward<Cb>(cb)(std::forward<F>(f)(std::forward<Args>(args)...));
            }
        }
    }
}
