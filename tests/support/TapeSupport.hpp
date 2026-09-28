// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <dxfeed_graal_cpp_api/api.hpp>

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace dxfcpp::test {

/// Returns the path of a file in `tests/data`.
std::string testDataPath(const std::string &relativePath);

/// Returns a unique path for a temporary file of the current test process (the file is not created).
std::string temporaryFilePath(const std::string &name);

/**
 * Reads a tape file (QD text or binary format) with a STREAM_FEED endpoint and returns its events in the order of the
 * file. Subscribes to all event types with the wildcard symbol and to the given candle symbols.
 * The Order-like events that QD derives from other records (the composite and regional sources, for example from
 * Quote) are skipped.
 */
std::vector<std::shared_ptr<EventType>> readTape(const std::string &path,
                                                 const std::vector<std::string> &candleSymbols = {});

/// Writes the events to a text tape file with a PUBLISHER endpoint. Rethrows the exception of the publisher.
void writeTextTape(const std::string &path, const std::vector<std::shared_ptr<EventType>> &events);

/**
 * A record of a QD text tape: the record name (e.g. `Quote`, `Order#NTV`), the fields by column name and, for the time
 * fields, the UTC offset they were written with (`+0300`).
 */
struct TapeRecord {
    std::string name;
    std::map<std::string, std::string> fields;
    std::map<std::string, std::string> timeOffsets;
};

/**
 * Parses the data lines of a QD text tape. Time values (`yyyyMMdd-HHmmss[.SSS]+hhmm` or `-hhmm`) are converted to UTC
 * milliseconds, so that the result does not depend on the time zone of the process that has written the file.
 * The named values after the columns (`EventFlags=...`) are fields too.
 */
std::vector<TapeRecord> parseTextTape(const std::string &path);

/**
 * Compares two text tapes. The records are matched by the record name and the event symbol, EventTime is ignored.
 * Returns one line per difference: `Record.Column@SYMBOL: expected=..., actual=...`, `Record@SYMBOL: missing` or
 * `Record@SYMBOL: unexpected`.
 */
std::vector<std::string> compareTapes(const std::string &expectedPath, const std::string &actualPath);

/**
 * Returns whether a difference of compareTapes() is known: a known entry is `Record.Column` (any symbol),
 * `Record.Column@SYMBOL` or `Record@SYMBOL`.
 */
bool isKnownDifference(const std::string &difference, const std::vector<std::string> &known);

/**
 * Checks that every expected event is among the actual events with the same toString(), which prints all the fields
 * of an event. The events are matched by the type, the order source and the event symbol.
 */
void checkEvents(const std::vector<std::shared_ptr<EventType>> &expected,
                 const std::vector<std::shared_ptr<EventType>> &actual);

/// Checks that there are no differences, except the known ones unless `withKnown` is set.
void checkDifferences(const std::vector<std::string> &differences, const std::vector<std::string> &known,
                      bool withKnown);

} // namespace dxfcpp::test
