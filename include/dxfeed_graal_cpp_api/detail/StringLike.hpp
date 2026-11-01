// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// Types declared here appear in public signatures and may be used through them; their names and this header's location
// are not a stable API.

#pragma once

#include "./Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <cstddef>
#include <functional>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <type_traits>

DXFCPP_BEGIN_NAMESPACE

/// A lightweight wrapper around strings or string-like inputs.
/// Stores either a view or an owned string but always exposes a valid view.
struct DXFCPP_EXPORT StringLike {
    mutable std::string owned_;
    mutable std::string_view view_;

    StringLike();

    // ReSharper disable once CppNonExplicitConvertingConstructor
    StringLike(const char *s);

    // ReSharper disable once CppNonExplicitConvertingConstructor
    StringLike(std::string_view sv);

    // ReSharper disable once CppNonExplicitConvertingConstructor
    StringLike(const std::string &s);

    // ReSharper disable once CppNonExplicitConvertingConstructor
    StringLike(std::string &&s) noexcept;

    StringLike(const StringLike &other);

    StringLike(StringLike &&other) noexcept;

    StringLike &operator=(const StringLike &other);

    StringLike &operator=(StringLike &&other) noexcept;

    // ReSharper disable once CppNonExplicitConvertingConstructor
    template <std::size_t N> StringLike(const char (&arr)[N]) : view_(arr, N - 1) {
    }

    // ReSharper disable once CppNonExplicitConversionOperator
    operator std::string_view() const noexcept;

    // ReSharper disable once CppNonExplicitConversionOperator
    operator std::string() const;

    const char *data() const noexcept;

    const char *c_str() const;

    std::string_view::const_iterator begin() const noexcept;

    std::string_view::const_iterator end() const noexcept;

    std::string_view::const_iterator cbegin() const noexcept;

    std::string_view::const_iterator cend() const noexcept;

    bool empty() const noexcept;

    std::size_t size() const noexcept;

    std::size_t length() const noexcept;

    bool ends_with(const StringLike &other) const noexcept;

    std::string substr(std::size_t pos = 0, std::size_t count = std::string::npos) const;

    bool operator==(const StringLike &other) const noexcept;

    friend std::string operator+(const StringLike &a, const StringLike &b) {
        std::string result;

        result.reserve(a.size() + b.size());
        result.append(a.view_);
        result.append(b.view_);

        return result;
    }

    explicit operator double() const;

    std::string toString() const noexcept;

    std::string_view toStringView() const noexcept;

    friend std::ostream &operator<<(std::ostream &os, const StringLike &sl) {
        return os << sl.view_;
    }
};

DXFCPP_END_NAMESPACE

template <> struct DXFCPP_EXPORT std::hash<dxfcpp::StringLike> {
    std::size_t operator()(const dxfcpp::StringLike &sl) const noexcept {
        return std::hash<std::string_view>{}(std::string_view(sl));
    }
};

DXFCPP_BEGIN_NAMESPACE

/// Universal functional object that allows searching std::unordered_map for string-like keys.
struct DXFCPP_EXPORT StringHash {
    using HashType = std::hash<std::string_view>;
    using is_transparent = void;

    std::size_t operator()(const char *str) const;
    std::size_t operator()(std::string_view sv) const;
    std::size_t operator()(const std::string &str) const;
    std::size_t operator()(const StringLike &s) const;
};

/**
 * Common string constants used across the library.
 *
 * `EMPTY` is an empty string value.
 * `NUL` is a textual placeholder for a missing or null string value
 * and is intended for logging, debugging, and diagnostic output.
 */
struct DXFCPP_EXPORT String {
    /// Empty string constant: `""`.
    static constexpr char EMPTY[] = "";

    /// Textual representation of a null / missing string: `"<null>"`.
    static constexpr char NUL[] = "<null>";
};

/**
 * The argument of the overloads of the string setters of events that also set null: `std::optional<std::string>` or
 * `std::nullopt`. Other arguments, e.g. strings and string literals, select the overloads that take dxfcpp::StringLike.
 */
template <typename T>
concept OptionalString = std::is_same_v<std::remove_cvref_t<T>, std::optional<std::string>> ||
                         std::is_same_v<std::remove_cvref_t<T>, std::nullopt_t>;

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()
