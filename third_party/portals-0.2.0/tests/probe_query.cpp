// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include "query.hpp"

#include <chrono>
#include <iostream>
#include <random>
#include <string>

namespace {
int failures = 0;

void check(bool condition, const char *name) {
  if (!condition) {
    std::cerr << "FAIL: " << name << '\n';
    ++failures;
  }
}
} // namespace

int main() {
  using portals::probe::QueryCollector;
  using namespace std::chrono_literals;
  const auto start = QueryCollector::Time{};
  const std::string input = "x\x1b[?1;0c\x1b[12;34Ry";

  for (std::size_t split = 0; split <= input.size(); ++split) {
    QueryCollector collector(start + 100ms, 128);

    collector.feed(std::string_view(input).substr(0, split), start + 1ms);
    collector.feed(std::string_view(input).substr(split), start + 2ms);
    collector.finish();
    check(collector.deviceAttributes() == "\x1b[?1;0c", "DA across every chunk boundary");
    check(collector.cursorPosition() == "\x1b[12;34R", "CPR across every chunk boundary");
    check(collector.unmatched() == "xy", "already-read unmatched bytes stay separate");
  }

  QueryCollector bytes(start + 100ms, 128);

  for (const char byte : std::string("\x1b[?62;1;6c\x1b[7;19R")) {
    bytes.feed(std::string_view(&byte, 1), start + 1ms);
  }

  check(bytes.deviceAttributes() == "\x1b[?62;1;6c" && bytes.cursorPosition() == "\x1b[7;19R",
        "bytewise fragmented replies");
  check(!bytes.active(start + 2ms), "complete collector stops waiting");
  bytes.feed("already-read user input", start + 3ms);
  check(bytes.unmatched() == "already-read user input", "completion stops waits without losing already-read chunks");

  QueryCollector native_batch(start + 100ms, 128);
  const std::string native_records = "\x1b[?1;0c\x1b[12;34Ruser";

  for (const char byte : native_records) {
    native_batch.feed(std::string_view(&byte, 1), start + 1ms);
  }

  native_batch.finish();
  check(native_batch.unmatched() == "user", "Win32-style bytewise records after replies remain unmatched");

  QueryCollector timeout(start + 100ms, 128);

  timeout.feed("a\x1b[?1;", start + 99ms);
  timeout.feed("0c", start + 100ms);
  timeout.finish();
  check(!timeout.deviceAttributes() && timeout.unmatched() == "a\x1b[?1;", "deadline rejects late chunk");
  check(timeout.received() == 6 && !timeout.active(start), "finish flushes incomplete sequence once");
  timeout.finish();
  check(timeout.unmatched() == "a\x1b[?1;", "finish is idempotent");

  QueryCollector slow(start + 10ms, 128);

  slow.feed("\x1b[", start + 1ms);
  slow.feed("?1;", start + 9ms);
  slow.feed("0c", start + 11ms);
  slow.finish();
  check(!slow.deviceAttributes(), "progress never extends total deadline");

  const std::string garbage = "\x1b[0;1R\x1b[1;0R\x1b[1;2;3R\x1b[?1;c\x1b[>1;0c\x1b[?42949672960c\x1b[A\x1b]0;evil\a";
  QueryCollector invalid(start + 100ms, 1024);

  invalid.feed(garbage, start);
  invalid.finish();
  check(!invalid.deviceAttributes() && !invalid.cursorPosition() && invalid.unmatched() == garbage,
        "malformed, unrelated and overflowing replies remain unmatched");

  QueryCollector duplicate(start + 100ms, 128);

  duplicate.feed("\x1b[?1;0c\x1b[?2c", start);
  duplicate.finish();
  check(duplicate.deviceAttributes() == "\x1b[?1;0c" && duplicate.unmatched() == "\x1b[?2c",
        "first reply wins and duplicates stay unmatched");

  QueryCollector limited(start + 100ms, 5);

  limited.feed("abc\x1b[?1;0c", start);
  check(limited.limited() && limited.received() == 5 && limited.unmatched() == "abc\x1b[",
        "global byte cap includes incomplete sequences");
  limited.feed("more", start);
  check(limited.received() == 5 && !limited.active(start), "cap never admits additional input");

  QueryCollector zero(start + 100ms, 0);

  zero.feed("input", start);
  check(!zero.active(start) && zero.received() == 0, "zero capacity admits no bytes");

  const std::string long_sequence = "\x1b[?" + std::string(200, '1') + "c";
  QueryCollector long_reply(start + 100ms, 1024);

  long_reply.feed(long_sequence, start);
  long_reply.finish();
  check(!long_reply.deviceAttributes() && long_reply.unmatched() == long_sequence, "sequence size is bounded");
  check(portals::probe::escapeBytes(std::string("\x1b\n\r\0\xff\"\\", 7)) == "\"\\x1b\\x0a\\x0d\\x00\\xff\\\"\\\\\"",
        "report escaping cannot emit terminal controls");

  std::mt19937 random(003);

  for (int iteration = 0; iteration < 500; ++iteration) {
    std::string raw;

    for (int index = 0; index < 257; ++index) {
      raw += static_cast<char>(random() & 0xff);
    }

    QueryCollector fuzz(start + 100ms, 113);

    for (std::size_t offset = 0; offset < raw.size(); offset += 7) {
      fuzz.feed(std::string_view(raw).substr(offset, 7), start + 1ms);
    }

    fuzz.finish();

    const auto retained =
      fuzz.unmatched().size() + fuzz.deviceAttributes().value_or("").size() + fuzz.cursorPosition().value_or("").size();

    check(retained == fuzz.received() && retained <= 113, "retained byte conservation and cap under garbage");

    for (const unsigned char value : portals::probe::escapeBytes(raw)) {
      check(value >= 0x20 && value <= 0x7e, "escaped random report is printable ASCII");
    }
  }

  if (failures == 0) {
    std::cout << "PASS: query boundaries, matching, fake-clock deadlines, garbage, limits and escaping\n";
  }

  return failures == 0 ? 0 : 1;
}
