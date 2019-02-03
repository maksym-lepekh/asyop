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

#include <variant>
#include <system_error>
#include <optional>


template <typename T>
struct voe: std::variant<T, std::error_code>
{
    using std::variant<T, std::error_code>::variant;

    bool has_value()
    {
        return this->index() == 0;
    }

    bool has_value() const
    {
        return this->index() == 0;
    }


    T& value()
    {
        return std::get<0>(*this);
    }

    std::error_code& error()
    {
        return std::get<1>(*this);
    }
};

template <typename T>
struct von: public std::optional<T>
{
    using std::optional<T>::optional;
};

struct noe: std::variant<std::monostate, std::error_code>
{
    using std::variant<std::monostate, std::error_code>::variant;

    bool has_value()
    {
        return this->index() == 0;
    }

    std::error_code& error()
    {
        return std::get<1>(*this);
    }
};
