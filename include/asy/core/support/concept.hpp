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

namespace asy::detail::concept
{
    template <bool> struct boolean;

    template <> struct boolean<true>
    {
        using is_true_t = void;
    };

    template <> struct boolean<false>
    {
        using is_false_t = void;
    };

    template <typename...>
    struct require_t{ using type = void; };
}

namespace asy::concept
{
    template <bool B>
    using is_true = typename detail::concept::boolean<B>::is_true_t;

    template <bool B>
    using is_false = typename detail::concept::boolean<B>::is_false_t;

    template <typename... Ts>
    using require = typename detail::concept::require_t<Ts...>::type;

    template<typename Concept, typename... T>
    using satisfy = require<decltype(std::declval<Concept>().impl(std::declval<T>()...))>;
}

namespace asy::detail::concept
{
    template<typename Concept, typename Sfinae = void>
    struct as_constant : std::false_type {};

    template<typename Concept, typename... T>
    struct as_constant<Concept(T...), asy::concept::satisfy<Concept, T...>> : std::true_type {};
}

namespace asy::concept
{
    template<typename Concept, typename... T>
    constexpr auto satisfies = detail::concept::as_constant<Concept(T...)>::value;
}

namespace asy
{
    namespace c = asy::concept;
}
