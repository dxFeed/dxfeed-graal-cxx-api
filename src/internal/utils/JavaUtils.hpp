// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../../include/dxfeed_graal_cpp_api/internal/Conf.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

DXFCPP_BEGIN_NAMESPACE

namespace java {

/**
 * Parses a string as Java `Double.parseDouble` does, independently of the C locale: leading and trailing characters
 * `<= ' '` are trimmed, an optional sign, `NaN`, `Infinity`, decimal and hexadecimal (`0x1p3`) literals, and an
 * optional `f`, `F`, `d` or `D` suffix. Overflow gives an infinity, underflow gives zero.
 *
 * @param s The string to parse.
 * @return The value, or `std::nullopt` if Java would throw `NumberFormatException`.
 */
std::optional<double> parseDouble(std::string_view s);

/**
 * Converts a double to a long as the Java cast `(long) value` does: NaN gives 0, values out of range saturate.
 */
std::int64_t longCast(double value) noexcept;

/**
 * Formats a double as Java `Double.toString` does: `NaN`, `Infinity`, `-Infinity`, plain notation (`100.0`, `0.001`)
 * for magnitudes in [1e-3, 1e7), otherwise `1.0E30` notation; the shortest digits that round-trip, at least two.
 */
std::string doubleToString(double value);

/**
 * Formats a double as the Java candle attributes do: `Long.toString((long) value)` when the value equals its long
 * cast (`5`, `0`), otherwise `Double.toString(value)` (`1.5`, `NaN`).
 */
std::string longOrDoubleToString(double value);

} // namespace java

DXFCPP_END_NAMESPACE
