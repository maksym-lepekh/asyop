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

#include "asy/common/type_traits.hpp"
#include "asy/common/value_or_error.hpp"
#include <functional>
#include <type_traits>

namespace asy
{
    template <typename T, typename Err>
    class basic_op_handle;

    template <typename T, typename Err>
    class basic_context;
}

namespace asy::detail
{
    enum struct cont_type
    {
        simple,
        areturn,
        async,
        ambiguous_simple,
        ambiguous_areturn,
        invalid
    };


    template <typename F, typename Input, typename... Aux>
    constexpr cont_type get_cont_type_input()
    {
        using info = functor_info<F>;
        if constexpr (info::arg_n > 0)
        {
            if (std::is_invocable_v<F, Input&&, Aux...>)
            {
                using ret_t = typename info::ret_type;
                if constexpr (specialization_of<basic_op_handle, ret_t>::value)
                    return cont_type::areturn;
                else
                    return cont_type::simple;
            }
            else
            {
                using is_shared_ptr = specialization_of<std::shared_ptr, typename info::arg1_type>;
                if constexpr (is_shared_ptr::value)
                {
                    if constexpr (specialization_of<basic_context, typename is_shared_ptr::first_arg>::value)
                    {
                        constexpr bool is_call_with_context = std::is_invocable_v<F, typename info::arg1_type, Input&&, Aux...>;
                        if constexpr (is_call_with_context && std::is_same_v<typename info::ret_type, void>)
                            return cont_type::async;
                    }
                }
            }
        }
        return cont_type::invalid;
    }

    template <typename F, typename... Aux>
    constexpr cont_type get_cont_type_void()
    {
        using info = functor_info<F>;
        if constexpr (info::arg_n == sizeof...(Aux))
        {
            using ret_t = typename info::ret_type;
            if constexpr (specialization_of<basic_op_handle, ret_t>::value)
                return cont_type::areturn;
            else
                return cont_type::simple;
        }
        else
        {
            using is_shared_ptr = specialization_of<std::shared_ptr, typename info::arg1_type>;
            if constexpr (is_shared_ptr::value)
            {
                if constexpr (specialization_of<basic_context, typename is_shared_ptr::first_arg>::value)
                {
                    constexpr bool is_call_with_context = std::is_invocable_v<F, typename info::arg1_type, Aux...>;
                    if constexpr (is_call_with_context && std::is_same_v<typename info::ret_type, void>)
                        return cont_type::async;
                }
            }
        }
        return cont_type::invalid;
    }

    template <typename F, typename Input, typename... Aux>
    constexpr cont_type get_cont_type_ambiguous()
    {
        if constexpr (std::is_invocable_v<F, Input&&, Aux...>)
        {
            using ret_t = std::invoke_result_t<F, Input&&, Aux...>;
            if constexpr (specialization_of<basic_op_handle, ret_t>::value)
                return cont_type::ambiguous_areturn;
            else
                return cont_type::ambiguous_simple;
        }
        return cont_type::invalid;
    }

    template <typename F, typename Input, typename... Aux>
    constexpr cont_type get_cont_type()
    {
        using info = functor_info<F>;
        if constexpr (info::is_ambiguous)
        {
            if constexpr (std::is_same_v<Input, void> && (sizeof...(Aux) == 0))
                return cont_type::invalid;
            else
                return get_cont_type_ambiguous<F, Input, Aux...>();
        }
        else
        {
            if constexpr (std::is_same_v<Input, void>)
                return get_cont_type_void<F, Aux...>();
            else
                return get_cont_type_input<F, Input, Aux...>();
        }
    }

    template <typename, bool>
    struct voe_unwrap_impl;

    template <typename Ret>
    struct voe_unwrap_impl<Ret, false>
    {
        static constexpr auto voe = false;
        static constexpr auto voe_type = voe_t::simple;
        using ret_type = Ret;
    };

    template <typename Ret>
    struct voe_unwrap_impl<Ret, true>
    {
        static constexpr auto voe = true;
        static constexpr auto voe_type = ValueOrError<Ret>::voe_type;
        using ret_type = typename ValueOrError<Ret>::success_type;
        using err_type = typename ValueOrError<Ret>::failure_type;
    };

    template <typename Ret>
    using voe_unwrap = voe_unwrap_impl<Ret, is_ValueOrError<Ret>>;

    template <cont_type Type>
    struct cont_info_base
    {
        static constexpr auto type = Type;
    };

    template <typename F, typename Input, cont_type Type, typename... Aux>
    struct cont_info_typed: cont_info_base<Type>
    {
        static constexpr auto type = cont_type::invalid;
    };

    template <typename F, typename Input, typename... Aux>
    struct cont_info_typed<F, Input, cont_type::simple, Aux...>:
            cont_info_base<cont_type::simple>,
            voe_unwrap<typename functor_info<F>::ret_type>
    {
        using ret_type_orig = typename functor_info<F>::ret_type;
    };

    template <typename F, typename Input, typename... Aux>
    struct cont_info_typed<F, Input, cont_type::areturn, Aux...>: cont_info_base<cont_type::areturn>
    {
        using ret_type = typename specialization_of<basic_op_handle, typename functor_info<F>::ret_type>::first_arg;
        using ret_type_orig = typename functor_info<F>::ret_type;
    };

    template <typename F, typename Input, typename... Aux>
    struct cont_info_typed<F, Input, cont_type::async, Aux...>: cont_info_base<cont_type::async>
    {
        using _shptr = typename functor_info<F>::arg1_type;
        using _ctx = typename specialization_of<std::shared_ptr, _shptr>::first_arg;
        using ret_type = typename specialization_of<basic_context, _ctx>::first_arg;
        using ret_type_orig = void;
    };

    template <typename F, typename Input, typename... Aux>
    struct cont_info_typed<F, Input, cont_type::ambiguous_simple, Aux...>:
            cont_info_base<cont_type::ambiguous_simple>,
            voe_unwrap<std::invoke_result_t<F, Input&&, Aux...>>
    {
        using ret_type_orig = std::invoke_result_t<F, Input&&, Aux...>;
    };

    template <typename F, typename Input, typename... Aux>
    struct cont_info_typed<F, Input, cont_type::ambiguous_areturn, Aux...>: cont_info_base<cont_type::ambiguous_areturn>
    {
        using ret_type = typename specialization_of<basic_op_handle, std::invoke_result_t<F, Input&&, Aux...>>::first_arg;
        using ret_type_orig = std::invoke_result_t<F, Input&&, Aux...>;
    };


    template <typename F, typename Input, bool IsStdFun, typename... Aux>
    struct cont_info_unwrap;

    template <typename F, typename Input, typename... Aux>
    struct cont_info_unwrap<F, Input, true, Aux...>: cont_info_typed<
            typename specialization_of<std::function, F>::first_arg,
            Input,
            get_cont_type<typename specialization_of<std::function, F>::first_arg, Input, Aux...>(),
            Aux...>{};

    template <typename F, typename Input, typename... Aux>
    struct cont_info_unwrap<F, Input, false, Aux...>: cont_info_typed<F, Input, get_cont_type<F, Input, Aux...>(), Aux...>{};


    template <typename F, typename Input, typename... Aux>
    struct continuation_info: cont_info_unwrap<F, Input, specialization_of<std::function, F>::value, Aux...>{};
}
