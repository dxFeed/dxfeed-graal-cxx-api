// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "JavaUtils.hpp"

#include "../../../include/dxfeed_graal_cpp_api/internal/Common.hpp"

#include <algorithm>
#include <array>
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

std::string longOrDoubleToString(double value) {
    const auto longValue = longCast(value);

    return math::exactlyEquals(value, static_cast<double>(longValue)) ? std::to_string(longValue)
                                                                      : doubleToString(value);
}

namespace {

// A port of the conversion of a double and of the formatting of com.devexperts.util.WideDecimal (dxlib). A wide
// decimal is a long with the significand in the higher 56 bits and the rank (128 + the number of digits after the
// point) in the lower 8 bits; rank 0 is NaN or an infinity, by the sign of the significand.
namespace wide {

constexpr std::int64_t MAX_SIGNIFICAND = std::numeric_limits<std::int64_t>::max() >> 8;
constexpr std::int64_t MIN_SIGNIFICAND = std::numeric_limits<std::int64_t>::min() >> 8;
constexpr int BIAS = 128;
constexpr int MAX_LEADING_ZEROES = 6;
constexpr int MAX_TRAILING_ZEROES = 6;
constexpr int EXACT_LONG_POWERS = 18; // max power of 10 that fits into long exactly
constexpr std::int64_t NaN = 0;
constexpr std::int64_t POSITIVE_INFINITY = 0x100;
constexpr std::int64_t NEGATIVE_INFINITY = -0x100;
constexpr std::array<std::int64_t, 4> NF_WIDE{NaN, POSITIVE_INFINITY, NaN, NEGATIVE_INFINITY};
constexpr std::array<const char *, 4> NF_STRING{"NaN", "Infinity", "NaN", "-Infinity"};

constexpr std::array<std::int64_t, EXACT_LONG_POWERS + 1> LONG_POWERS = [] {
    std::array<std::int64_t, EXACT_LONG_POWERS + 1> powers{1};

    for (std::size_t i = 1; i < powers.size(); i++) {
        powers[i] = powers[i - 1] * 10;
    }

    return powers;
}();

constexpr std::int64_t SCIENTIFIC_MODULO = LONG_POWERS[MAX_TRAILING_ZEROES + 1];

// DIVISORS[rank] is 1e(rank - 128) and MAX_DOUBLES[rank] is 2^51 * 1e(128 - rank), both parsed as Java parses them;
// DIVISORS[0] is 0 and MAX_DOUBLES[0] is infinity.
struct Tables {
    std::array<double, 256> divisors{};
    std::array<double, 256> maxDoubles{};

    Tables() noexcept {
        constexpr auto maxDoubleValue = static_cast<double>(std::int64_t{1} << 51);

        maxDoubles[0] = std::numeric_limits<double>::infinity();

        for (int rank = 1; rank < 256; rank++) {
            divisors[rank] = strtodC("1E" + std::to_string(rank - BIAS));
            maxDoubles[rank] = maxDoubleValue * strtodC("1E" + std::to_string(BIAS - rank));
        }
    }
};

const Tables &tables() noexcept {
    static const Tables instance{};

    return instance;
}

// Long.signum(value) & 3 of Java: the index of NaN, an infinity or a negative infinity in NF_WIDE and NF_STRING.
std::size_t signumIndex(std::int64_t value) noexcept {
    return value > 0 ? 1 : value < 0 ? 3 : 0;
}

std::int64_t neg(std::int64_t wide) noexcept {
    return static_cast<std::int64_t>((static_cast<std::uint64_t>(wide) ^ ~std::uint64_t{0xFF}) + 0x100);
}

// Divides the significand by a power of 10 (10+) with rounding, without overflows near the limits.
std::int64_t div10(std::int64_t significand, std::int64_t pow10) noexcept {
    return significand >= 0 ? (significand / (pow10 >> 1) + 1) >> 1 : ((significand + 1) / (pow10 >> 1)) >> 1;
}

std::int64_t composeFitting(std::int64_t significand, int rank, int targetRank) noexcept {
    if (significand == 0) {
        return targetRank;
    }

    while (rank > targetRank && significand % 10 == 0) {
        significand /= 10;
        rank--;
    }

    while (rank < targetRank && MIN_SIGNIFICAND / 10 <= significand && significand <= MAX_SIGNIFICAND / 10) {
        significand *= 10;
        rank++;
    }

    if (rank == BIAS + 1 && targetRank == BIAS && MIN_SIGNIFICAND / 10 <= significand &&
        significand <= MAX_SIGNIFICAND / 10) {
        significand *= 10;
        rank++;
    }

    return (significand << 8) | rank;
}

std::int64_t composeNonFitting(std::int64_t significand, int rank, int targetRank) noexcept {
    if (rank <= 0) {
        return NF_WIDE[signumIndex(significand)];
    }

    if (rank > 255 + EXACT_LONG_POWERS) {
        return targetRank;
    }

    int reduction = 3;

    if (MIN_SIGNIFICAND <= significand && significand <= MAX_SIGNIFICAND) {
        reduction = 0;
    } else if (MIN_SIGNIFICAND * 10 - 5 <= significand && significand < MAX_SIGNIFICAND * 10 + 5) {
        reduction = 1;
    } else if (MIN_SIGNIFICAND * 100 - 50 <= significand && significand < MAX_SIGNIFICAND * 100 + 50) {
        reduction = 2;
    }

    if (rank <= reduction) {
        return NF_WIDE[signumIndex(significand)];
    }

    reduction = std::max(reduction, rank - 255);

    if (reduction > 0) {
        significand = div10(significand, LONG_POWERS[reduction]);
        rank -= reduction;
    }

    return composeFitting(significand, rank, targetRank);
}

std::int64_t composeLong(std::int64_t value) noexcept {
    return MIN_SIGNIFICAND <= value && value <= MAX_SIGNIFICAND ? (value << 8) | BIAS
                                                                : composeNonFitting(value, BIAS, BIAS);
}

int findRank(double value) noexcept {
    const auto &maxDoubles = tables().maxDoubles;
    int rank = value > maxDoubles[128] ? 0 : 128;

    for (int step = 64; step > 0; step /= 2) {
        rank += value > maxDoubles[rank + step] ? 0 : step;
    }

    return rank;
}

std::int64_t composeNonNegative(double value, int targetRank) noexcept {
    const int rank = findRank(value);

    if (rank <= 0) {
        return POSITIVE_INFINITY;
    }

    return composeFitting(longCast(value * tables().divisors[rank] + 0.5), rank, targetRank);
}

std::int64_t composeWide(double value) noexcept {
    if (const auto longValue = longCast(value); math::exactlyEquals(value, static_cast<double>(longValue))) {
        return composeLong(longValue);
    }

    if (value >= 0) {
        return composeNonNegative(value, BIAS);
    }

    if (value < 0) {
        return neg(composeNonNegative(-value, BIAS));
    }

    return NaN;
}

// The significand as an integer without a fraction, or 0 if the number has a fraction or needs the E notation.
std::int64_t toIntegerString(std::int64_t significand, int rank) noexcept {
    if (rank > BIAS) {
        // fractional number with possible zero fraction and more trailing zeroes
        if (rank - BIAS <= EXACT_LONG_POWERS) {
            const auto pow10 = LONG_POWERS[rank - BIAS];
            const auto result = significand / pow10;

            if (result * pow10 == significand && result % SCIENTIFIC_MODULO != 0) {
                return result;
            }
        }
    } else if (rank == BIAS) {
        return significand % SCIENTIFIC_MODULO == 0 ? 0 : significand;
    } else if (BIAS - rank <= MAX_TRAILING_ZEROES) {
        static_assert(MAX_TRAILING_ZEROES <= EXACT_LONG_POWERS);

        // integer number with possible trailing zeroes; the product wraps around as in Java and is checked
        const auto pow10 = LONG_POWERS[BIAS - rank];
        const auto result =
            static_cast<std::int64_t>(static_cast<std::uint64_t>(significand) * static_cast<std::uint64_t>(pow10));

        if (result / pow10 == significand && result % SCIENTIFIC_MODULO != 0) {
            return result;
        }
    }

    return 0;
}

std::string toFractionalOrScientificString(std::int64_t significand, int rank) {
    // remove trailing zeroes
    while (significand % 10 == 0) {
        significand /= 10;
        rank--;
    }

    // the position of the first digit, after a potential '-' sign
    const int firstDigit = significand < 0 ? 1 : 0;
    std::string result = std::to_string(significand);

    // use plain decimal number notation unless scientific notation is triggered
    if (rank > BIAS) {
        // fractional number
        const int dotPosition = static_cast<int>(result.size()) - (rank - BIAS);

        if (dotPosition > firstDigit) {
            return result.insert(static_cast<std::size_t>(dotPosition), 1, '.');
        }

        if (firstDigit - dotPosition <= MAX_LEADING_ZEROES) {
            return result.insert(static_cast<std::size_t>(firstDigit),
                                 "0." + std::string(static_cast<std::size_t>(firstDigit - dotPosition), '0'));
        }
    } else if (BIAS - rank <= MAX_TRAILING_ZEROES) {
        // integer number
        return result.append(static_cast<std::size_t>(BIAS - rank), '0');
    }

    // use scientific notation
    const int digits = static_cast<int>(result.size()) - firstDigit;

    if (digits != 1) {
        result.insert(static_cast<std::size_t>(firstDigit) + 1, 1, '.');
    }

    return result + "E" + std::to_string(BIAS - rank + digits - 1);
}

std::string toString(std::int64_t wide) {
    const auto significand = wide >> 8;
    const auto rank = static_cast<int>(wide & 0xFF);

    if (rank == 0) {
        return NF_STRING[signumIndex(significand)];
    }

    if (significand == 0) {
        return "0";
    }

    // if value is an integer number - use faster formatting method
    if (const auto integer = toIntegerString(significand, rank); integer != 0) {
        return std::to_string(integer);
    }

    return toFractionalOrScientificString(significand, rank);
}

} // namespace wide

} // namespace

std::string wideDecimalToString(double value) {
    return wide::toString(wide::composeWide(value));
}

} // namespace java

DXFCPP_END_NAMESPACE
