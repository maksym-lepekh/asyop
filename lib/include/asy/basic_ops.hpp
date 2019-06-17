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
#include <memory>
#include <variant>
#include <optional>
#include "basic_op.hpp"

namespace asy::detail
{
    template<typename F, std::size_t... S>
    constexpr void static_for(F&& function, std::index_sequence<S...>) {
        (... , function(std::integral_constant<std::size_t, S>{}));
    }

    template<std::size_t iterations, typename F>
    constexpr void static_for(F&& function) {
        static_for(std::forward<F>(function), std::make_index_sequence<iterations>());
    }

    template<typename In, typename Err>
    using out_var_t = std::variant<std::monostate, In, Err>;

    template <typename T, typename Err>
    auto make_skip()
    {
        if constexpr (std::is_void_v<T>)
            return [](basic_context_ptr<T, Err> ctx){ ctx->async_success(); };
        else
            return [](basic_context_ptr<T, Err> ctx, T&& input){ ctx->async_success(std::move(input)); };
    }
}

namespace asy
{

    template <typename T, typename Err, typename Fn>
    auto add_cancel(asy::basic_op_handle<T, Err>& handle, Fn&& fn)
    {
        return handle.then(detail::make_skip<T, Err>(),
        [cb = std::forward<Fn>(fn)](basic_context_ptr<void, Err> ctx, Err&& err)
        {
            if (err == detail::error_traits<Err>::get_cancelled())
            {
                cb();
            }
            ctx->async_failure(std::move(err));
        });
    }

    template <typename Err, typename... Fs>
    auto basic_when_all(Fs&&...fs)
    {
        using ops_t = std::tuple<decltype(basic_op<Err>(std::declval<Fs>()))...>;
        using rets_t = std::tuple<detail::out_var_t<typename decltype(basic_op<Err>(std::declval<Fs>()))::output_t, Err>...>;

        auto ops = std::make_shared<ops_t>(basic_op<Err>(std::forward<Fs>(fs))...);

        auto h = basic_op_handle<rets_t, Err>([ops](basic_context_ptr<rets_t, Err> ctx, Fs&&...fs)
        {
            auto counter = std::make_shared<int>(sizeof...(Fs));
            auto res = std::make_shared<rets_t>();

            detail::static_for<sizeof...(Fs)>([&](auto index)
            {
                std::get<index.value>(*ops).then(
                        [res, counter, ctx, index](auto&& output) {
                            std::get<index.value>(*res) = std::forward<decltype(output)>(output);
                            if (--(*counter) == 0)
                            {
                                ctx->async_success(std::move(*res));
                            }
                        },
                        [res, counter, ctx, index](auto&& err) {
                            std::get<index.value>(*res) = std::forward<decltype(err)>(err);
                            if (--(*counter) == 0)
                            {
                                ctx->async_success(std::move(*res));
                            }
                        });
            });
        }, std::forward<Fs>(fs)...);

        return add_cancel(h, [ops]()
        {
            detail::static_for<sizeof...(Fs)>([&](auto idx)
            {
                std::get<idx.value>(*ops).cancel();
            });
        });
    }

    template <typename Err, typename... Fs>
    auto basic_when_success(Fs&&...fs)
    {
        using ops_t = std::tuple<decltype(basic_op<Err>(std::declval<Fs>()))...>;
        using rets_t = std::tuple<typename decltype(basic_op<Err>(std::declval<Fs>()))::output_t...>;

        auto ops = std::make_shared<ops_t>(basic_op<Err>(std::forward<Fs>(fs))...);

        auto h = basic_op_handle<rets_t, Err>([ops](basic_context_ptr<rets_t, Err> ctx, Fs&&...fs)
        {
            auto counter = std::make_shared<int>(sizeof...(Fs));
            auto res = std::make_shared<rets_t>();

            detail::static_for<sizeof...(Fs)>([&](auto index)
            {
                std::get<index.value>(*ops).then(
                      [res, counter, ctx, index](auto&& output) {
                          std::get<index.value>(*res) = std::forward<decltype(output)>(output);
                          if (--(*counter) == 0)
                          {
                              ctx->async_success(std::move(*res));
                          }
                      },
                      [res, counter, ctx, index, ops](auto&& err) {
                          ctx->async_failure(std::forward<decltype(err)>(err));
                          detail::static_for<sizeof...(Fs)>([&](auto idx)
                          {
                              if (idx != index) std::get<idx.value>(*ops).cancel();
                          });
                      });
            });
        }, std::forward<Fs>(fs)...);

        return add_cancel(h, [ops]()
        {
            detail::static_for<sizeof...(Fs)>([&](auto idx)
            {
                std::get<idx.value>(*ops).cancel();
            });
        });
    }

    template <typename Err, typename... Fs>
    auto basic_when_any(Fs&&...fs)
    {
        using ops_t = std::tuple<decltype(basic_op<Err>(std::declval<Fs>()))...>;
        using rets_t = std::variant<typename decltype(basic_op<Err>(std::declval<Fs>()))::output_t...>;

        auto ops = std::make_shared<ops_t>(basic_op<Err>(std::forward<Fs>(fs))...);

        auto h = basic_op_handle<rets_t, Err>([ops](basic_context_ptr<rets_t, Err> ctx, Fs&&...fs)
        {
            detail::static_for<sizeof...(Fs)>([&](auto index){
                std::get<index.value>(*ops).then(
                        [ctx, index, ops](auto&& output) {
                            ctx->async_success(rets_t(std::in_place_index<index.value>, std::forward<decltype(output)>(output)));
                            detail::static_for<sizeof...(Fs)>([&](auto idx)
                            {
                                if (idx != index) std::get<idx.value>(*ops).cancel();
                            });
                        },
                        [ctx, index, ops](auto&& err) {
                            ctx->async_failure(std::forward<decltype(err)>(err));
                            detail::static_for<sizeof...(Fs)>([&](auto idx)
                            {
                                if (idx != index) std::get<idx.value>(*ops).cancel();
                            });
                        });
            });
        }, std::forward<Fs>(fs)...);

        return add_cancel(h, [ops]()
        {
            detail::static_for<sizeof...(Fs)>([&](auto idx)
            {
                std::get<idx.value>(*ops).cancel();
            });
        });
    }
}
