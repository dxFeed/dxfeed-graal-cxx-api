// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/internal/Common.hpp"

#include "../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"

DXFCPP_BEGIN_NAMESPACE

namespace day_util {
std::int32_t getDayIdByYearMonthDay(std::int32_t year, std::int32_t month, std::int32_t day) {
    if (month < 1 || month > 12) {
        throw InvalidArgumentException("invalid month " + std::to_string(month));
    }

    std::int32_t dayOfYear = DAY_OF_YEAR[month] + day - 1;

    if (month > 2 && year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) {
        dayOfYear++;
    }

    return year * 365 + math_util::div(year - 1, 4) - math_util::div(year - 1, 100) + math_util::div(year - 1, 400) +
           dayOfYear - 719527;
}
} // namespace day_util

namespace util {

void throwInvalidChar(char c, const std::string &name) {
    throw InvalidArgumentException("Invalid " + name + ": " + encodeChar(c));
}

} // namespace util

DXFCPP_END_NAMESPACE
