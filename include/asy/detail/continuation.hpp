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

#include "type_traits.hpp"
#include <functional>
#include <type_traits>

namespace asy
{
    template <typename T>
    class op_handle;

    template <typename T, typename Err>
    class context;
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


    template <typename F, typename Input>
    constexpr cont_type get_cont_type_input()
    {
        using info = functor_info<F>;
        if constexpr (info::arg_n > 0)
        {
            if ((info::arg_n == 1) && (std::is_same_v<typename info::arg1_type, Input&&>))
            {
                using ret_t = typename info::ret_type;
                if constexpr (specialization_of<op_handle, ret_t>::value)
                    return cont_type::areturn;
                else
                    return cont_type::simple;
            }
            else
            {
                using is_shared_ptr = specialization_of<std::shared_ptr, typename info::arg1_type>;
                if constexpr (is_shared_ptr::value)
                {
                    if constexpr (specialization_of<context, typename is_shared_ptr::first_arg>::value)
                    {
                        constexpr bool is_call_with_context = std::is_invocable_v<F, typename info::arg1_type, Input&&>;
                        if constexpr (is_call_with_context && std::is_same_v<typename info::ret_type, void>)
                            return cont_type::async;
                    }
                }
            }
        }
        return cont_type::invalid;
    }

    template <typename F>
    constexpr cont_type get_cont_type_void()
    {
        using info = functor_info<F>;
        if constexpr (info::arg_n == 0)
        {
            using ret_t = typename info::ret_type;
            if constexpr (specialization_of<op_handle, ret_t>::value)
                return cont_type::areturn;
            else
                return cont_type::simple;
        }
        else
        {
            using is_shared_ptr = specialization_of<std::shared_ptr, typename info::arg1_type>;
            if constexpr (is_shared_ptr::value)
            {
                if constexpr (specialization_of<context, typename is_shared_ptr::first_arg>::value)
                {
                    constexpr bool is_call_with_context = std::is_invocable_v<F, typename info::arg1_type>;
                    if constexpr (is_call_with_context && std::is_same_v<typename info::ret_type, void>)
                        return cont_type::async;
                }
            }
        }
        return cont_type::invalid;
    }

    template <typename F, typename Input>
    constexpr cont_type get_cont_type_ambiguous()
    {
        if constexpr (std::is_invocable_v<F, Input&&>)
        {
            using ret_t = std::invoke_result_t<F, Input&&>;
            if constexpr (specialization_of<op_handle, ret_t>::value)
                return cont_type::ambiguous_areturn;
            else
                return cont_type::ambiguous_simple;
        }
        return cont_type::invalid;
    }

    template <typename F, typename Input>
    constexpr cont_type get_cont_type()
    {
        using info = functor_info<F>;
        if constexpr (info::is_ambiguous)
        {
            if constexpr (std::is_same_v<Input, void>)
                return cont_type::invalid;
            else
                return get_cont_type_ambiguous<F, Input>();
        }
        else
        {
            if constexpr (std::is_same_v<Input, void>)
                return get_cont_type_void<F>();
            else
                return get_cont_type_input<F, Input>();
        }
    }

    template <cont_type Type>
    struct cont_info_base
    {
        static constexpr auto type = Type;
    };

    template <typename F, typename Input, cont_type Type>
    struct cont_info_typed: cont_info_base<Type>
    {
        static constexpr auto type = cont_type::invalid;
    };

    template <typename F, typename Input>
    struct cont_info_typed<F, Input, cont_type::simple>: cont_info_base<cont_type::simple>
    {
        using ret_type = typename functor_info<F>::ret_type;
    };

    template <typename F, typename Input>
    struct cont_info_typed<F, Input, cont_type::areturn>: cont_info_base<cont_type::areturn>
    {
        using ret_type = typename specialization_of<op_handle, typename functor_info<F>::ret_type>::first_arg;
    };

    template <typename F, typename Input>
    struct cont_info_typed<F, Input, cont_type::async>: cont_info_base<cont_type::async>
    {
        using _shptr = typename functor_info<F>::arg1_type;
        using _ctx = typename specialization_of<std::shared_ptr, _shptr>::first_arg;
        using ret_type = typename specialization_of<context, _ctx>::first_arg;
    };

    template <typename F, typename Input>
    struct cont_info_typed<F, Input, cont_type::ambiguous_simple>: cont_info_base<cont_type::ambiguous_simple>
    {
        using ret_type = std::invoke_result_t<F, Input&&>;
    };

    template <typename F, typename Input>
    struct cont_info_typed<F, Input, cont_type::ambiguous_areturn>: cont_info_base<cont_type::ambiguous_areturn>
    {
        using ret_type = typename specialization_of<op_handle, std::invoke_result_t<F, Input&&>>::first_arg;
    };


    template <typename F, typename Input, bool IsStdFun>
    struct cont_info_unwrap;

    template <typename F, typename Input>
    struct cont_info_unwrap<F, Input, true>: cont_info_typed<
            typename specialization_of<std::function, F>::first_arg,
            Input,
            get_cont_type<typename specialization_of<std::function, F>::first_arg, Input>()>{};

    template <typename F, typename Input>
    struct cont_info_unwrap<F, Input, false>: cont_info_typed<F, Input, get_cont_type<F, Input>()>{};


    template <typename F, typename Input>
    struct continuation_info: cont_info_unwrap<F, Input, specialization_of<std::function, F>::value>{};
}
