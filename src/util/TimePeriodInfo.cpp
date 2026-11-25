// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/util/TimePeriodInfo.hpp"

#include "../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../internal/utils/JavaUtils.hpp"

#include <cstddef>
#include <string>

#include <fmt/format.h>

DXFCPP_BEGIN_NAMESPACE

constinit const TimePeriodInfo TimePeriodInfo::UNKNOWN{-1000, -1000};
constinit const TimePeriodInfo TimePeriodInfo::EMPTY{0, 0};

namespace {

// Formats milliseconds as seconds the way Java `FastDoubleUtil.formatDoubleScale(millis / 1000.0, 3)` does: plain
// notation below 1e10 seconds (`0.25`, `86400`), otherwise `1.5E10` notation; at most 3 fraction digits.
std::string formatSeconds(std::int64_t millis) {
    if (millis == 0) {
        return "0";
    }

    // 1e10 seconds. Below it a double keeps every millisecond, so the integer arithmetic gives the same digits.
    constexpr std::int64_t plainLimit = 10'000'000'000'000;

    if (millis > -plainLimit && millis < plainLimit) {
        const auto magnitude = millis < 0 ? -millis : millis;
        auto result = (millis < 0 ? "-" : "") + std::to_string(magnitude / 1000);

        if (const auto fraction = magnitude % 1000; fraction != 0) {
            auto fractionDigits = fmt::format("{:03}", fraction);

            fractionDigits.erase(fractionDigits.find_last_not_of('0') + 1);
            result += "." + fractionDigits;
        }

        return result;
    }

    // The shortest digits of the double, as in "-1.2345678901234568E13".
    const auto text = java::doubleToString(static_cast<double>(millis) / 1000.0);
    const auto negative = text.front() == '-';
    const auto start = static_cast<std::size_t>(negative);
    const auto ePos = text.find('E');
    auto digits = text.substr(start, ePos - start);
    auto exponent = std::stoi(text.substr(ePos + 1));

    digits.erase(1, 1); // the point

    // Scale 3: the digits after the third fraction digit are rounded half up.
    if (const auto precision = static_cast<std::size_t>(exponent) + 4; digits.size() > precision) {
        const auto roundUp = digits[precision] >= '5';

        digits.resize(precision);

        if (roundUp) {
            auto i = digits.size();

            while (i > 0 && digits[i - 1] == '9') {
                digits[--i] = '0';
            }

            if (i == 0) {
                digits.insert(digits.begin(), '1');
                exponent++;
            } else {
                digits[i - 1]++;
            }
        }
    }

    if (const auto lastNonZero = digits.find_last_not_of('0'); lastNonZero != std::string::npos) {
        digits.erase(lastNonZero + 1);
    }

    return (negative ? "-" : "") + digits.substr(0, 1) + (digits.size() > 1 ? "." + digits.substr(1) : "") + "E" +
           std::to_string(exponent);
}

} // namespace

TimePeriodInfo TimePeriodInfo::valueOf(std::int64_t min, std::int64_t max) {
    if (min < 0 || max < 0) {
        throw InvalidArgumentException(fmt::format("min and max must be non-negative, got: ({},{})", min, max));
    }

    if (min > max) {
        throw InvalidArgumentException(fmt::format("min ({}) must be less than or equal to max ({})", min, max));
    }

    if (min == 0 && max == 0) {
        return EMPTY;
    }

    return {min, max};
}

std::string TimePeriodInfo::toString() const {
    return R"({"min":)" + formatSeconds(min_) + R"(,"max":)" + formatSeconds(max_) + "}";
}

DXFCPP_END_NAMESPACE
