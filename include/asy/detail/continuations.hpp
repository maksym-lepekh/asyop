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

namespace asy::detail
{
    template <typename Input, typename Output, voe_t VoE, typename Fn, typename Ctx>
    auto make_simple_cont(Fn&& fn, Ctx parent_ctx)
    {
        if constexpr (std::is_void_v<Input>)
        {
            return [fn = std::forward<Fn>(fn), parent_ctx](){
                if constexpr (std::is_void_v<Output>){
                    fn();
                    parent_ctx->async_success();
                } else if constexpr (VoE == voe_t::voe) {
                    auto&& ret = fn();
                    if (ret.has_value())
                        parent_ctx->async_success(std::move(ret.value()));
                    else
                        parent_ctx->async_failure(std::move(ret.error()));
                } else if constexpr (VoE == voe_t::von) {
                    auto&& ret = fn();
                    if (ret.has_value())
                        parent_ctx->async_success(std::move(ret.value()));
                    else
                        parent_ctx->async_failure();
                } else if constexpr (VoE == voe_t::noe) {
                    auto&& ret = fn();
                    if (ret.has_value())
                        parent_ctx->async_success();
                    else
                        parent_ctx->async_failure(std::move(ret.error()));
                } else {
                    parent_ctx->async_success(fn());
                }
            };
        }
        else
        {
            return [fn = std::forward<Fn>(fn), parent_ctx](Input&& input)
            {
                if constexpr (std::is_void_v<Output>){
                    fn(std::move(input));
                    parent_ctx->async_success();
                } else if constexpr (VoE == voe_t::voe) {
                    auto&& ret = fn(std::move(input));
                    if (ret.has_value())
                        parent_ctx->async_success(std::move(ret.value()));
                    else
                        parent_ctx->async_failure(std::move(ret.error()));
                } else if constexpr (VoE == voe_t::von) {
                    auto&& ret = fn(std::move(input));
                    if (ret.has_value())
                        parent_ctx->async_success(std::move(ret.value()));
                    else
                        parent_ctx->async_failure();
                } else if constexpr (VoE == voe_t::noe) {
                    auto&& ret = fn(std::move(input));
                    if (ret.has_value())
                        parent_ctx->async_success();
                    else
                        parent_ctx->async_failure(std::move(ret.error()));
                } else {
                    parent_ctx->async_success(fn(std::move(input)));
                }
            };
        }
    }

    template <typename Input, voe_t VoE, typename Fn, typename Ctx>
    auto make_simple_failcont(Fn&& fn, Ctx parent_ctx)
    {
        return [fn = std::forward<Fn>(fn), parent_ctx](Input&& err){
            if constexpr (VoE == voe_t::voe || VoE == voe_t::noe)
            {
                auto&& ret = fn(std::move(err));
                if (ret.has_value())
                    parent_ctx->async_success();
                else
                    parent_ctx->async_failure(std::move(ret.error()));
            }
            if constexpr (VoE == voe_t::von)
            {
                auto&& ret = fn(std::move(err));
                if (ret.has_value())
                    parent_ctx->async_success();
                else
                    parent_ctx->async_failure();
            }
            else
            {
                fn(std::move(err));
                parent_ctx->async_success();
            }
        };
    }

    template <typename Input, typename Output, typename Fn, typename Ctx>
    auto make_areturn_cont(Fn&& fn, Ctx parent_ctx)
    {
        if constexpr (std::is_void_v<Input>)
        {
            return [fn = std::forward<Fn>(fn), parent_ctx]() {
                auto handle = fn();
                if constexpr (std::is_void_v<Output>) {
                    handle.then([parent_ctx]() { parent_ctx->async_success(); },
                            [parent_ctx](auto&& err) { parent_ctx->async_failure(std::forward<decltype(err)>(err)); });
                } else {
                    handle.then([parent_ctx](Output&& output) { parent_ctx->async_success(std::move(output)); },
                            [parent_ctx](auto&& err) { parent_ctx->async_failure(std::forward<decltype(err)>(err)); });
                }
            };
        }
        else
        {
            return [fn = std::forward<Fn>(fn), parent_ctx](Input&& input) {
                auto handle = fn(std::move(input));
                if constexpr (std::is_void_v<Output>) {
                    handle.then([parent_ctx]() { parent_ctx->async_success(); },
                                [parent_ctx](auto&& err) { parent_ctx->async_failure(std::forward<decltype(err)>(err)); });
                } else {
                    handle.then([parent_ctx](Output&& output) { parent_ctx->async_success(std::move(output)); },
                                [parent_ctx](auto&& err) { parent_ctx->async_failure(std::forward<decltype(err)>(err)); });
                }
            };
        }
    }

    template <typename Input, typename Fn, typename Ctx>
    auto make_areturn_failcont(Fn&& fn, Ctx parent_ctx)
    {
        return [fn = std::forward<Fn>(fn), parent_ctx](Input&& err){
            auto handle = fn(std::move(err));
            handle.then([parent_ctx](){ parent_ctx->async_success(); });
        };
    }

    template <typename Input, typename Output, typename Op, typename Fn, typename Ctx>
    auto make_async_cont(Fn&& fn, Ctx parent_ctx)
    {
        if constexpr (std::is_void_v<Input>)
        {
            return [fn = std::forward<Fn>(fn), parent_ctx](){
                if constexpr (std::is_void_v<Output>)
                {
                    Op(fn).then(
                            [parent_ctx](){ parent_ctx->async_success(); },
                            [parent_ctx](auto&& err){ parent_ctx->async_failure(std::forward<decltype(err)>(err)); });
                }
                else
                {
                    Op(fn).then(
                            [parent_ctx](Output&& output){ parent_ctx->async_success(std::move(output)); },
                            [parent_ctx](auto&& err){ parent_ctx->async_failure(std::forward<decltype(err)>(err)); });
                }
            };
        }
        else
        {
            return [fn = std::forward<Fn>(fn), parent_ctx](Input&& input){
                if constexpr (std::is_void_v<Output>)
                {
                    Op(fn, std::move(input)).then(
                            [parent_ctx](){ parent_ctx->async_success(); },
                            [parent_ctx](auto&& err){ parent_ctx->async_failure(std::forward<decltype(err)>(err)); });
                }
                else
                {
                    Op(fn, std::move(input)).then(
                            [parent_ctx](Output&& output){ parent_ctx->async_success(std::move(output)); },
                            [parent_ctx](auto&& err){ parent_ctx->async_failure(std::forward<decltype(err)>(err)); });
                }
            };
        }
    }

    template <typename Input, typename Op, typename Fn, typename Ctx>
    auto make_async_failcont(Fn&& fn, Ctx parent_ctx)
    {
        if constexpr (std::is_void_v<Input>)
        {
            return [fn = std::forward<Fn>(fn), parent_ctx](){
                Op(fn).then(
                        [parent_ctx](){ parent_ctx->async_success(); },
                        [parent_ctx](auto&& err){ parent_ctx->async_failure(std::forward<decltype(err)>(err)); });
            };
        }
        else
        {
            return [fn = std::forward<Fn>(fn), parent_ctx](Input&& input){
                Op(fn, std::move(input)).then(
                        [parent_ctx](){ parent_ctx->async_success(); },
                        [parent_ctx](auto&& err){ parent_ctx->async_failure(std::forward<decltype(err)>(err)); });
            };
        }

    }

    template <typename Input, typename Ctx>
    static auto make_skip_cont(Ctx ctx)
    {
        if constexpr (std::is_void_v<Input>)
        {
            return [ctx](){ ctx->async_success(); };
        }
        else
        {
            return [ctx](Input&& input){ ctx->async_success(); };
        }
    }

    template <typename Input, typename Ctx>
    static auto make_skip_failcont(Ctx ctx)
    {
        return [ctx](Input&& err)
        {
            ctx->async_failure(std::move(err));
        };
    }
}
