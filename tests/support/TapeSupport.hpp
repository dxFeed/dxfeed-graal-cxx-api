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

/// Writes the events to a text tape file with a PUBLISHER endpoint.
void writeTextTape(const std::string &path, const std::vector<std::shared_ptr<EventType>> &events);

/// A record of a QD text tape: the record name (e.g. `Quote`, `Order#NTV`) and the fields by column name.
struct TapeRecord {
    std::string name;
    std::map<std::string, std::string> fields;
};

/**
 * Parses the data lines of a QD text tape. Time values (`yyyyMMdd-HHmmss[.SSS]±hhmm`) are converted to UTC
 * milliseconds, so that the result does not depend on the time zone of the process that has written the file.
 */
std::vector<TapeRecord> parseTextTape(const std::string &path);

} // namespace dxfcpp::test
