// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// This header is not part of the API. It is installed only because public headers need it; its contents may change
// without notice.

#pragma once

#include "./Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <concepts>
#include <iterator>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

DXFCPP_BEGIN_NAMESPACE

template <typename T>
concept Integral = std::is_integral_v<T>;

template <class From, class To>
concept ConvertibleTo = std::is_convertible_v<From, To> && requires { static_cast<To>(std::declval<From>()); };

template <class T, class U>
concept Derived = std::is_base_of_v<U, T>;

template <class T, class U>
concept BaseOf = Derived<U, T>;

template <typename T>
concept PairLike = requires(T t) {
    t.first;
    t.second;
};

template <typename T>
concept MapLike = requires(T t) {
    { std::begin(t) } -> std::input_iterator;
    { std::end(t) } -> std::sentinel_for<decltype(std::begin(t))>;
} && PairLike<std::remove_cvref_t<decltype(*std::begin(std::declval<T &>()))>>;

/**
 * The argument of the overloads of the string setters of events that also set null: `std::optional<std::string>` or
 * `std::nullopt`. Other arguments, e.g. strings and string literals, select the overloads that take
 * dxfcpp::StringParam.
 */
template <typename T>
concept OptionalString = std::is_same_v<std::remove_cvref_t<T>, std::optional<std::string>> ||
                         std::is_same_v<std::remove_cvref_t<T>, std::nullopt_t>;

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()
