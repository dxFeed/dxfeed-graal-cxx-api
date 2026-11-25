// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../detail/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <chrono>
#include <cstdint>

/**
 * \addtogroup dxfcpp_util
 * @{
 */

DXFCPP_BEGIN_NAMESPACE

/**
 * Returns the current time.
 *
 * @return The number of milliseconds that have passed since the start of the Unix epoch (1970-01-01).
 */
inline auto now() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

/// Functions of the day identifiers: the number of days since Unix epoch of January 1, 1970.
namespace day_util {

/**
 * Returns the day identifier for the specified year, month and day in Gregorian calendar.
 * The day identifier is defined as the number of days since Unix epoch of January 1, 1970.
 * Month must be between 1 and 12 inclusive. Year and day might take arbitrary values assuming proleptic Gregorian
 * calendar.
 *
 * @param year The year.
 * @param month The month between 1 and 12 inclusive.
 * @param day The day.
 * @return The day identifier.
 * @throws InvalidArgumentException if the month is less than 1 or more than 12.
 */
DXFCPP_EXPORT std::int32_t getDayIdByYearMonthDay(std::int32_t year, std::int32_t month, std::int32_t day);

} // namespace day_util

DXFCPP_END_NAMESPACE

/// @}

DXFCXX_DISABLE_MSC_WARNINGS_POP()
