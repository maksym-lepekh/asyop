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

namespace asy::detail
{
    enum struct voe_t
    {
        simple,
        voe,    ///< ValueOrError
        noe,    ///< NoneOrError
        von     ///< ValueOrNone
    };

    template <typename T>
    struct dummy{};

    template <typename T>
    class voe_detect
    {
    public:
        template <typename C>
        static constexpr bool check_has_value(dummy<decltype(std::declval<C>().has_value())>*) {
            return std::is_constructible_v<decltype(std::declval<C>().has_value()), bool>;
        }

        template <typename>
        static constexpr bool check_has_value(...) { return false; }

        template <typename C>
        static constexpr bool check_value(dummy<decltype(std::declval<C>().value())>*) { return true; }

        template <typename>
        static constexpr bool check_value(...) { return false; }

        template <typename C>
        static constexpr bool check_error(dummy<decltype(std::declval<C>().error())>*) {
            return !std::is_void_v<decltype(std::declval<T>().error())>;
        }

        template <typename>
        static constexpr bool check_error(...) { return false; }

    public:
        static constexpr voe_t detect() noexcept
        {
            if constexpr (check_has_value<T>(nullptr))
            {
                constexpr auto val = check_value<T>(nullptr);
                constexpr auto err = check_error<T>(nullptr);

                if constexpr (val && err)
                {
                    if constexpr (std::is_void_v<decltype(std::declval<T>().value())>)
                        return voe_t::noe;
                    else
                        return voe_t::voe;
                }
                else if constexpr (val)
                {
                    if constexpr (!std::is_void_v<decltype(std::declval<T>().value())>)
                        return voe_t::von;
                }
                else if constexpr (err)
                {
                    return voe_t::noe;
                }
            }

            return voe_t::simple;
        }
    };

    template <typename, voe_t>
    struct value_or_error;

    template <typename T>
    struct value_or_error<T, voe_t::simple>: std::false_type{};

    template <typename T>
    struct value_or_error<T, voe_t::voe>: std::true_type{
        static constexpr auto voe_type = voe_t::voe;
        using success_type = std::remove_cv_t<std::remove_reference_t<decltype(std::declval<T>().value())>>;
        using failure_type = std::remove_cv_t<std::remove_reference_t<decltype(std::declval<T>().error())>>;
    };

    template <typename T>
    struct value_or_error<T, voe_t::von>: std::true_type{
        static constexpr auto voe_type = voe_t::von;
        using success_type = std::remove_cv_t<std::remove_reference_t<decltype(std::declval<T>().value())>>;
        using failure_type = void;
    };

    template <typename T>
    struct value_or_error<T, voe_t::noe>: std::true_type{
        static constexpr auto voe_type = voe_t::noe;
        using success_type = void;
        using failure_type = std::remove_cv_t<std::remove_reference_t<decltype(std::declval<T>().error())>>;
    };

    // pretend to be concept
    template <typename T>
    using ValueOrError = value_or_error<T, voe_detect<T>::detect()>;

    template <typename T>
    constexpr auto is_ValueOrError = ValueOrError<T>::value;
}
