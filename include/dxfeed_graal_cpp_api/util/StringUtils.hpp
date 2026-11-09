// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../detail/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>

/**
 * \addtogroup dxfcpp_util
 * @{
 */

DXFCPP_BEGIN_NAMESPACE

/// Universal functional object that allows searching std::unordered_map for string-like keys.
struct DXFCPP_EXPORT StringHash {
    using HashType = std::hash<std::string_view>;
    using is_transparent = void;

    std::size_t operator()(const char *str) const;
    std::size_t operator()(std::string_view sv) const;
    std::size_t operator()(const std::string &str) const;
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
 * Converts a double value to a string in the format of Java `Double.toString`: the shortest decimal representation
 * that identifies the value, `1.0` for 1, `1.0E-4` for 0.0001, `NaN`, `Infinity`, `-Infinity`, `-0.0`.
 *
 * @param d The value.
 * @return The string.
 */
DXFCPP_EXPORT std::string toString(double d); // NOLINT(*-redundant-declaration)

DXFCPP_END_NAMESPACE

/// @}

DXFCXX_DISABLE_MSC_WARNINGS_POP()
