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

/// The symbols of the value sets of createEdgeEvents(): `EDGE-NAN`, `EDGE-ZERO`, ..., one per set.
std::vector<std::string> edgeSymbols();

/// The candle symbols (`<edge symbol>{=d}`) of the Candle events created by createEdgeEvents().
std::vector<std::string> edgeCandleSymbols();

/**
 * Creates the events of all 18 types for every value set of edgeSymbols() and 256 orders with every value of the event
 * flags byte (`EDGE-FLAGS-000` ... `EDGE-FLAGS-255`). In a value set every double field gets the same edge value (NaN,
 * +0.0, -0.0, infinities, the largest, the lowest and the smallest subnormal values, a fraction, a large number with a
 * fraction, a small negative number), the time and long fields get the int64 limits and other values, the int fields
 * get the int32 limits, the strings get an empty string, `"<null>"`, `\NULL`, non-ASCII UTF-8, tabs and line feeds, a
 * long string and escaped characters, the enums cycle through all their values. Only the values that the Java API
 * publishes are used: orders do not get the UNDEFINED side, the QD short strings (market maker, sale conditions) get
 * at most 8 ASCII characters.
 *
 * The events match `tests/data/tapes/edge-values.txt`, which is written by the Java API from the same values.
 */
std::vector<std::shared_ptr<EventType>> createEdgeEvents();

} // namespace dxfcpp::test
