// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <string_view>

namespace portals::probe {

/// Tool-private, single-owner query collector. All captured strings own their bytes.
class QueryCollector {
public:
  using Time = std::chrono::steady_clock::time_point;

  /// Creates a collection window; byte_limit bounds all retained input together.
  QueryCollector(Time deadline, std::size_t byte_limit);
  /// Classifies bytes received before deadline; later bytes are not consumed.
  /// Already-read chunks remain classifiable after both replies are matched.
  void feed(std::string_view bytes, Time now);
  /// Ends the window and classifies incomplete sequences as unmatched input.
  void finish();
  /// Returns false at the deadline, on completion, or when the byte limit is hit.
  bool active(Time now) const;
  /// Observed primary DA reply; matching is syntactic and not authentication.
  const std::optional<std::string> &deviceAttributes() const;
  /// Observed CPR reply; keyboard input may have an identical encoding.
  const std::optional<std::string> &cursorPosition() const;
  /// Unmatched captured input, with no claim that it is all user-generated.
  const std::string &unmatched() const;
  /// Number of bytes consumed from the input stream.
  std::size_t received() const;
  /// True when further input was truncated at the configured byte limit.
  bool limited() const;

private:
  Time deadline_;
  std::size_t byte_limit_;
  std::size_t received_ = 0;
  bool finished_ = false;
  bool limited_ = false;
  std::string pending_;
  std::string unmatched_;
  std::optional<std::string> da_;
  std::optional<std::string> cpr_;
};

/// Produces printable ASCII with quoted strings and hex escapes for unsafe bytes.
std::string escapeBytes(std::string_view bytes);

} // namespace portals::probe
