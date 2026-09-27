// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <dxfeed_graal_cpp_api/api.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace dxfcpp::test {

/// The symbol of the events created by createTapeEvents().
inline constexpr auto TAPE_SYMBOL = "ROUND-TRIP";

/// The candle symbol of the Candle created by createTapeEvents().
inline std::string tapeCandleSymbol() {
    return std::string(TAPE_SYMBOL) + "{=d}";
}

/**
 * Creates one event of every event type (18 types). Every field that the default dxFeed scheme transfers (the columns
 * of the text tape) has its own value, so a lost, swapped or mangled field changes the event. Fields that the scheme
 * does not transfer (for example, Quote::getSequence() or the iceberg fields of AnalyticOrder) keep their defaults.
 *
 * The events match `tests/data/tapes/all-events.txt`.
 */
std::vector<std::shared_ptr<EventType>> createTapeEvents();

/// The symbol of the events created by createDefaultEvents().
inline constexpr auto DEFAULTS_SYMBOL = "DEFAULTS";

/**
 * Creates one default-constructed event of every event type with the DEFAULTS_SYMBOL symbol (the orders get
 * the NTV, GLBX, pink and ISE sources, the candle gets the `{=d}` period) and the event time 1700000000000.
 * The events match `tests/data/tapes/defaults.txt`, which is written by the Java API.
 */
std::vector<std::shared_ptr<EventType>> createDefaultEvents();

} // namespace dxfcpp::test
