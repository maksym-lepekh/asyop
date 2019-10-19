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
    /// The global facility that is capable of invocation of the callable on preferred thread. Used by
    /// operation context to run continuations
    namespace executor
    {
        /// Client-side callable type
        using fn_t = std::function<void()>;

        /// Per-thread handler type
        using impl_t = std::function<void(fn_t)>;

        /// Add a functor to execution queue. The functor invocation is expected to be done immediately
        /// and on a preferred thread
        ///
        /// \param fn Callable object
        /// \param id Preferred thread ID, optional, defaults to current thread
        void schedule_execution(fn_t fn, std::thread::id id = std::this_thread::get_id());

        /// Check whether the specified thread shares operation context with other threads, thus context access
        /// must be synchronised
        ///
        /// \param id Thread ID, optional, defaults to current thread
        /// \return True if the data access should be synchronized, False otherwise
        [[nodiscard]]
        bool should_sync(std::thread::id id = std::this_thread::get_id()) noexcept;

        /// Set the handler for specified thread ID. This handler is responsible for invocation of callables
        /// that are passed with `schedule_execution()`.
        /// \see schedule_execution()
        ///
        /// \param id Thread ID
        /// \param impl Handler
        /// \param require_sync Execution on the specified thread ID should synchronize data access
        void set_impl(std::thread::id id, impl_t impl, bool require_sync);
    };
}}
