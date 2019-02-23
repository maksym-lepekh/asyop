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

#include <functional>
#include <thread>

namespace asy { inline namespace v1
{
    struct executor
    {
        using fn_t = std::function<void()>;
        using impl_t = std::function<void(fn_t)>;

        void schedule_execution(fn_t fn, std::thread::id id = std::this_thread::get_id()) noexcept;
        bool should_sync(std::thread::id id = std::this_thread::get_id()) const noexcept;
        void set_impl(std::thread::id id, impl_t impl, bool require_sync);

        static executor& get() noexcept;

    private:
        executor();
    };
}}
