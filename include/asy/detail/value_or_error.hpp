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
    template <typename T>
    class is_value_or_error
    {
        template <typename C>
        static constexpr bool check_has_value(decltype(&C::has_value)) {
            return std::is_constructible_v<decltype(std::declval<T>().has_value()), bool>;
        }

        template <typename>
        static constexpr bool check_has_value(...) { return false; }

        template <typename C>
        static constexpr bool check_value(decltype(&C::value)) {
            return !std::is_void_v<decltype(std::declval<T>().value())>;
        }

        template <typename>
        static constexpr bool check_value(...) { return false; }

        template <typename C>
        static constexpr bool check_error(decltype(&C::error)) {
            return !std::is_void_v<decltype(std::declval<T>().error())>;
        }

        template <typename>
        static constexpr bool check_error(...) { return false; }

    public:
        static constexpr auto value = check_has_value<T>(nullptr) && check_value<T>(nullptr) && check_error<T>(nullptr);
    };

    template <typename, bool>
    struct value_or_error;

    template <typename T>
    struct value_or_error<T, false>: std::false_type{};

    template <typename T>
    struct value_or_error<T, true>: std::true_type{
        using success_type = decltype(std::declval<T>().value());
        using failure_type = decltype(std::declval<T>().error());
    };

    // pretend to be concept
    template <typename T>
    using ValueOrError = value_or_error<T, is_value_or_error<T>::value>;

    template <typename T>
    constexpr auto is_ValueOrError = ValueOrError<T>::value;
}
