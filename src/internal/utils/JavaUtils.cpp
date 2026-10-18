// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "JavaUtils.hpp"

#include "../../../include/dxfeed_graal_cpp_api/internal/Common.hpp"

#include <cmath>
#include <cstdlib>
#include <limits>
#include <locale.h>
#include <stdlib.h>
#include <utility>

#if defined(__APPLE__)
#    include <xlocale.h>
#endif

#include <fmt/format.h>

DXFCPP_BEGIN_NAMESPACE

namespace java {

namespace {

bool isDigit(char c) noexcept {
    return c >= '0' && c <= '9';
}

bool isHexDigit(char c) noexcept {
    return isDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

bool isTypeSuffix(char c) noexcept {
    return c == 'f' || c == 'F' || c == 'd' || c == 'D';
}

// Converts a string already checked against the Java grammar, so only the decimal point depends on the locale.
double strtodC(const std::string &s) noexcept {
#if defined(_WIN32)
    static const _locale_t locale = _create_locale(LC_NUMERIC, "C");

    return _strtod_l(s.c_str(), nullptr, locale);
#else
    static const locale_t locale = newlocale(LC_NUMERIC_MASK, "C", static_cast<locale_t>(nullptr));

    return strtod_l(s.c_str(), nullptr, locale);
#endif
}

// Skips the digits (hex digits) of [i, end) as `digits* ('.' digits*)?`; returns false unless there is a digit.
bool skipMantissa(std::string_view s, std::size_t &i, bool hex) noexcept {
    const auto isDigitOf = hex ? isHexDigit : isDigit;
    std::size_t digits = 0;

    for (; i < s.size() && isDigitOf(s[i]); i++) {
        digits++;
    }

    if (i < s.size() && s[i] == '.') {
        i++;

        for (; i < s.size() && isDigitOf(s[i]); i++) {
            digits++;
        }
    }

    return digits > 0;
}

// Skips `[+-]? digits+`; returns false if there is no digit.
bool skipExponentValue(std::string_view s, std::size_t &i) noexcept {
    if (i < s.size() && (s[i] == '+' || s[i] == '-')) {
        i++;
    }

    const auto start = i;

    for (; i < s.size() && isDigit(s[i]); i++) {
    }

    return i > start;
}

} // namespace

std::optional<double> parseDouble(std::string_view s) {
    // Java String.trim(): characters <= ' ' at both ends.
    while (!s.empty() && static_cast<unsigned char>(s.front()) <= ' ') {
        s.remove_prefix(1);
    }

    while (!s.empty() && static_cast<unsigned char>(s.back()) <= ' ') {
        s.remove_suffix(1);
    }

    std::size_t i = 0;
    bool negative = false;

    if (i < s.size() && (s[i] == '+' || s[i] == '-')) {
        negative = s[i] == '-';
        i++;
    }

    const auto unsignedPart = s.substr(i);

    if (unsignedPart == "NaN") {
        return std::numeric_limits<double>::quiet_NaN();
    }

    if (unsignedPart == "Infinity") {
        return negative ? -std::numeric_limits<double>::infinity() : std::numeric_limits<double>::infinity();
    }

    const bool hex =
        unsignedPart.size() > 1 && unsignedPart[0] == '0' && (unsignedPart[1] == 'x' || unsignedPart[1] == 'X');

    if (hex) {
        i += 2;
    }

    if (!skipMantissa(s, i, hex)) {
        return std::nullopt;
    }

    if (hex) {
        // The binary exponent is mandatory in a hexadecimal literal.
        if (i >= s.size() || (s[i] != 'p' && s[i] != 'P')) {
            return std::nullopt;
        }

        i++;

        if (!skipExponentValue(s, i)) {
            return std::nullopt;
        }
    } else if (i < s.size() && (s[i] == 'e' || s[i] == 'E')) {
        i++;

        if (!skipExponentValue(s, i)) {
            return std::nullopt;
        }
    }

    const auto numberEnd = i;

    if (i < s.size() && isTypeSuffix(s[i])) {
        i++;
    }

    if (i != s.size()) {
        return std::nullopt;
    }

    return strtodC(std::string(s.substr(0, numberEnd)));
}

std::int64_t longCast(double value) noexcept {
    // 2^63 is exactly representable as a double.
    constexpr double twoPow63 = 9223372036854775808.0;

    if (std::isnan(value)) {
        return 0;
    }

    if (value >= twoPow63) {
        return std::numeric_limits<std::int64_t>::max();
    }

    if (value <= -twoPow63) {
        return std::numeric_limits<std::int64_t>::min();
    }

    return static_cast<std::int64_t>(value);
}

namespace {

// The significant digits of a positive finite double as written by fmt and the decimal exponent of the first one:
// "1.5e+07" -> {"15", 7}, "0.001" -> {"1", -3}.
std::pair<std::string, int> splitDigits(const std::string &formatted) {
    const auto ePos = formatted.find_first_of("eE");
    const auto mantissa = formatted.substr(0, ePos);
    const int exponent = ePos == std::string::npos ? 0 : std::atoi(formatted.c_str() + ePos + 1);
    const auto pointPos = mantissa.find('.');
    auto digits = mantissa;
    auto pointIndex = static_cast<int>(mantissa.size());

    if (pointPos != std::string::npos) {
        digits.erase(pointPos, 1);
        pointIndex = static_cast<int>(pointPos);
    }

    const auto firstNonZero = digits.find_first_not_of('0');

    digits.erase(0, firstNonZero);
    pointIndex -= static_cast<int>(firstNonZero);

    const auto lastNonZero = digits.find_last_not_of('0');

    digits.erase(lastNonZero + 1);

    return {digits, pointIndex - 1 + exponent};
}

} // namespace

std::string doubleToString(double value) {
    if (std::isnan(value)) {
        return "NaN";
    }

    if (std::isinf(value)) {
        return value > 0 ? "Infinity" : "-Infinity";
    }

    const std::string sign = std::signbit(value) ? "-" : "";
    const auto magnitude = std::fabs(value);

    if (math::exactlyEquals(magnitude, 0.0)) {
        return sign + "0.0";
    }

    auto split = splitDigits(fmt::format("{}", magnitude));
    auto &digits = split.first;
    auto &exponent = split.second;

    // Java writes at least two significant digits; with only one shortest digit it takes the two-digit decimal
    // closest to the value if that still converts back to it (4.9E-324 rather than 5.0E-324).
    if (digits.size() == 1) {
        const auto twoDigits = fmt::format("{:.1e}", magnitude);

        if (math::exactlyEquals(strtodC(twoDigits), magnitude)) {
            split = splitDigits(twoDigits);
        }
    }

    std::string result = sign;

    if (exponent >= -3 && exponent < 7) {
        if (exponent >= 0) {
            const auto intDigits = static_cast<std::size_t>(exponent) + 1;

            if (digits.size() <= intDigits) {
                result += digits + std::string(intDigits - digits.size(), '0') + ".0";
            } else {
                result += digits.substr(0, intDigits) + "." + digits.substr(intDigits);
            }
        } else {
            result += "0." + std::string(static_cast<std::size_t>(-exponent - 1), '0') + digits;
        }
    } else {
        result +=
            digits.substr(0, 1) + "." + (digits.size() > 1 ? digits.substr(1) : "0") + "E" + std::to_string(exponent);
    }

    return result;
}

} // namespace java

DXFCPP_END_NAMESPACE
