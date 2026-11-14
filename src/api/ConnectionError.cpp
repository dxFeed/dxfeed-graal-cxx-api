// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/api/ConnectionError.hpp"

#include <fmt/format.h>

#include <cstdint>
#include <string>

DXFCPP_BEGIN_NAMESPACE

namespace {

// The time in milliseconds since the epoch as an ISO 8601 UTC time with milliseconds: 2026-01-02T03:04:05.678Z.
// Not TimeFormat: ConnectionError is a plain value, its toString() works without the GraalVM isolate (no SDK call).
std::string formatUtcMillis(std::int64_t time) {
    // The days since the epoch and the milliseconds of the day, both rounded down for a time before the epoch.
    constexpr std::int64_t millisPerDay = 86'400'000;
    auto days = time / millisPerDay;
    auto millisOfDay = time % millisPerDay;

    if (millisOfDay < 0) {
        millisOfDay += millisPerDay;
        days--;
    }

    // The civil date of the day (the algorithm days_from_civil inverted, proleptic Gregorian calendar).
    const auto z = days + 719'468;
    const auto era = (z >= 0 ? z : z - 146'096) / 146'097;
    const auto dayOfEra = z - era * 146'097;
    const auto yearOfEra = (dayOfEra - dayOfEra / 1'460 + dayOfEra / 36'524 - dayOfEra / 146'096) / 365;
    const auto dayOfYear = dayOfEra - (365 * yearOfEra + yearOfEra / 4 - yearOfEra / 100);
    const auto shiftedMonth = (5 * dayOfYear + 2) / 153;
    const auto day = dayOfYear - (153 * shiftedMonth + 2) / 5 + 1;
    const auto month = shiftedMonth < 10 ? shiftedMonth + 3 : shiftedMonth - 9;
    const auto year = yearOfEra + era * 400 + (month <= 2 ? 1 : 0);

    return fmt::format("{:04}-{:02}-{:02}T{:02}:{:02}:{:02}.{:03}Z", year, month, day, millisOfDay / 3'600'000,
                       millisOfDay / 60'000 % 60, millisOfDay / 1'000 % 60, millisOfDay % 1'000);
}

} // namespace

const char *ConnectionError::categoryToString(Category category) noexcept {
    switch (category) {
    case Category::OTHER:
        return "OTHER";
    case Category::DNS:
        return "DNS";
    case Category::CONNECT_FAILED:
        return "CONNECT_FAILED";
    case Category::READ_TIMEOUT:
        return "READ_TIMEOUT";
    case Category::HEARTBEAT_TIMEOUT:
        return "HEARTBEAT_TIMEOUT";
    case Category::CLOSED_BY_REMOTE:
        return "CLOSED_BY_REMOTE";
    case Category::TLS:
        return "TLS";
    case Category::IO:
        return "IO";
    }

    return "OTHER";
}

std::string ConnectionError::toString() const {
    auto result = fmt::format("ConnectionError{{time = {}, category = {}, dropped = {}, address = {}",
                              formatUtcMillis(time), categoryToString(category), dropped, address);

    // The exception and the message are left out when there are none.
    if (!exceptionClassName.empty()) {
        result += ", exception = " + exceptionClassName;
    }

    if (!message.empty()) {
        result += ", message = " + message;
    }

    result += fmt::format(", count = {}}}", count);

    for (auto &c : result) {
        if (c == '\n' || c == '\r') {
            c = ' ';
        }
    }

    return result;
}

DXFCPP_END_NAMESPACE
