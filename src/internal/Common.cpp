// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/internal/Common.hpp"

#include "../../include/dxfeed_graal_cpp_api/exceptions/ErrorManager.hpp"
#include "../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../../include/dxfeed_graal_cpp_api/internal/Isolate.hpp"

#include <exception>

DXFCPP_BEGIN_NAMESPACE

void detail::reportScopeExitException(const std::source_location &location) noexcept {
    if (!Isolate::isShuttingDown()) {
        ErrorManager::reportException("DXFCPP_FINALLY", std::current_exception(), location);
    }
}

namespace day_util {
std::int32_t getDayIdByYearMonthDay(std::int32_t year, std::int32_t month, std::int32_t day) {
    if (month < 1 || month > 12) {
        throw InvalidArgumentException("invalid month " + std::to_string(month));
    }

    // The arithmetic of the Java int: any year and day give the result of Java.
    using math_util::JavaInt;

    JavaInt dayOfYear = JavaInt{DAY_OF_YEAR[month]} + day - 1;

    if (month > 2 && year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) {
        dayOfYear = dayOfYear + 1;
    }

    const JavaInt y = year;
    const auto previous = (y - 1).value;

    return (y * 365 + math_util::div(previous, 4) - math_util::div(previous, 100) + math_util::div(previous, 400) +
            dayOfYear - 719527)
        .value;
}
} // namespace day_util

namespace util {

void throwInvalidChar(char c, const std::string &name) {
    throw InvalidArgumentException("Invalid " + name + ": " + encodeChar(c));
}

} // namespace util

DXFCPP_END_NAMESPACE
