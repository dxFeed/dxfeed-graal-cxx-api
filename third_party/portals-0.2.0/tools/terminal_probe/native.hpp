// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#pragma once

#include "query.hpp"

#include <string>
#include <utility>
#include <vector>

namespace portals::probe {

/// Tool-private native facts; values are escaped by the report writer.
struct NativeFacts {
  /// Owned diagnostic key/value strings; no terminal control output.
  std::vector<std::pair<std::string, std::string>> fields;
  /// True only when both standard handles expose the native terminal APIs.
  bool query_eligible = false;
};

/// Collection outcome; absence of replies is an observation, not an I/O error.
struct RunResult {
  /// Completion reason, including unsupported, deadline and input limit.
  std::string stop;
  /// Native error description; empty on successful observation.
  std::string error;
  /// Native non-byte input records consumed (Windows only).
  std::size_t non_byte_events = 0;
  /// Escaped-by-writer summary of bounded non-byte records (Windows only).
  std::string event_summary;
};

/// Reads native OS/terminal facts without changing modes; single calling owner.
NativeFacts inspectNative();
/// Owns query output/input and temporary modes on the calling thread.
/// All normal/error paths restore modes. The collector and result own input;
/// this standalone tool does not replay unmatched input to the parent shell.
RunResult runNativeQueries(QueryCollector &collector, QueryCollector::Time deadline, std::size_t event_limit);

} // namespace portals::probe
