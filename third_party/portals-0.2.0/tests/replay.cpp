// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include "support/test_terminal.hpp"

#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
using namespace portals;
using namespace test_support;
int failures{};

void check(bool condition, const char *message) {
  if (!condition) {
    std::cerr << message << '\n';
    ++failures;
  }
}

template <class Function> void rejects(Function action, std::string_view reason) {
  try {
    action();
    check(false, "invalid operation unexpectedly accepted");
  } catch (const std::runtime_error &error) {
    check(std::string_view(error.what()).find(reason) != std::string_view::npos,
          "diagnostic must identify rejection reason");
  }
}

std::string readFixture(const std::string &path) {
  std::ifstream stream(path, std::ios::binary);

  if (!stream) {
    throw std::runtime_error("cannot read fixture: " + path);
  }

  std::string bytes(TraceLimits::BYTES + 1, '\0');

  stream.read(bytes.data(), static_cast<std::streamsize>(bytes.size()));
  bytes.resize(static_cast<std::size_t>(stream.gcount()));

  return bytes;
}

std::string utf8(std::u8string_view value) {
  return {reinterpret_cast<const char *>(value.data()), value.size()};
}

void partitions(const Trace &trace, const ReplayResult &expected) {
  for (std::size_t input = 1; input <= 32; ++input) {
    for (std::size_t output = 1; output <= 16; ++output) {
      check(replay(trace, input, output) == expected, "trace must be independent of input/output chunks");
    }
  }

  check(parseTrace(recordTrace(trace)) == trace, "recording roundtrip retains all typed operations");
  check(replay(parseTrace(recordTrace(trace)), 1, 1) == expected, "recorded trace retains semantic evidence");
}

void fixtures(const std::string &root) {
  auto resize = parseTrace(readFixture(root + "/resize-input.trace"));
  auto result = replay(resize, TraceLimits::BYTES, TraceLimits::BYTES);
  const std::vector<ReplayEvent> expected{{0, CellSize{4, 2}},
                                          {1, CellSize{6, 2}},
                                          {1, InputEvent{KeyEvent{InputKey::UP}}},
                                          {2, InputEvent{TextInputEvent{U'界'}}},
                                          {33, InputEvent{KeyEvent{InputKey::ESCAPE}}},
                                          {34, InputEvent{PasteInputEvent{"ab"}}},
                                          {36, CellSize{5, 2}}};

  check(result.events == expected, "resize between CSI/UTF-8 fragments retains exact event/time order");
  check(result.snapshots.size() == 3, "explicit and final checkpoints retained");

  const CellStyle rich{{RgbColor{1, 2, 3}}, {RgbColor{9, 8, 7}}, true, true, true};
  const auto &before = result.snapshots.at(0);

  check(before.size == CellSize{6, 2} && before.cells.at(10) == VtOracle::Pixel{utf8(u8"界"), rich, 2, false} &&
          before.cells.at(11) == VtOracle::Pixel{utf8(u8"界"), rich, 0, true},
        "snapshot owns styled wide lead and continuation at right edge");
  check(before.spans.back() == VtOracle::Span{1, 4, 6} && before.cursor == CellPoint{} &&
          before.cursor_style == CellStyle{} && before.autowrap && !before.wrap_pending,
        "snapshot includes exact wide span and neutral cursor state");
  check(result.snapshots.at(1).cells.at(9) == VtOracle::Pixel{}, "resize clears clipped wide lead");
  check(result.snapshots.at(2).cells.at(0).glyph == utf8(u8"é") && result.snapshots.at(2).cursor == CellPoint{1, 0},
        "combining accent preserves cursor column");
  partitions(resize, result);

  const auto wide = parseTrace(readFixture(root + "/wide-edge.trace"));
  const auto edge = replay(wide, 99, 99);

  check(edge.snapshots.at(0).cursor == CellPoint{3, 0} && !edge.snapshots.at(0).autowrap &&
          !edge.snapshots.at(0).wrap_pending && edge.snapshots.at(0).cells.at(3).tail,
        "wide glyph fits final column without implicit wrap");
  check(edge.snapshots.at(1).cursor == CellPoint{} && edge.snapshots.at(1).autowrap,
        "explicit mode/cursor baseline is observable");
  partitions(wide, edge);

  const auto failure = parseTrace(readFixture(root + "/writer-failure.trace"));
  const auto failed = replay(failure, 99, 99);

  check(failed.writes.size() == 2 && failed.writes.at(1) == ReplayWrite{TerminalStatus::FAILED, 2, 5, true},
        "writer failure retains unsent suffix and accepted prefix count");
  check(failed.snapshots.at(0) == failed.snapshots.at(1) && failed.snapshots.at(1) == failed.snapshots.at(2),
        "failure inside CSI does not invent completed output");
  partitions(failure, failed);
}

void clockAndTransport() {
  FakeClock clock;
  TestTerminal terminal(clock, {4, 1}, 1);

  check(terminal.input("\x1b").empty(), "ESC waits for fake deadline");
  clock.advanceTo(29);
  check(terminal.tick().empty(), "timer does not fire early");
  clock.advanceTo(30);
  check(terminal.tick() == std::vector<InputEvent>{KeyEvent{InputKey::ESCAPE}}, "tick explicitly delivers timeout");
  rejects(
    [&] {
      clock.advanceTo(29);
    },
    "backward");
  rejects(
    [&] {
      clock.advanceTo(std::numeric_limits<InputTime>::max());
    },
    "unrepresentable");
  check(clock.now() == 30, "failed clock advance preserves time");
  clock.advanceTo(FakeClock::MAX_TIME);
  check(clock.deadline() <= TerminalDeadline::max(), "largest fake time is representable by output clock");

  FakeClock output_clock;
  TestTerminal output(output_clock, {4, 1});

  check(output.submit("ab").has_value(), "transport takes owning output");
  check(!output.submit("x"), "pending output rejects overwrite");
  check(output.flush(output_clock.deadline()).status == TerminalStatus::DEADLINE, "expired deadline accepts no bytes");

  std::stop_source stop;

  stop.request_stop();
  check(output.flush(TerminalDeadline::max(), stop.get_token()).status == TerminalStatus::STOPPED,
        "pre-stop accepts no bytes");
  check(output.snapshot().cells.at(0).glyph == " ", "submit and stopped/expired flush leave screen unchanged");
  check(output.flush(TerminalDeadline::max()).completed == 2, "later flush preserves exact pending suffix");

  const auto snapshot = output.snapshot();

  output.resize({2, 1});
  check(snapshot.size == CellSize{4, 1} && snapshot.cells.at(1).glyph == "b", "snapshot outlives mutation");
}

CellBuffer frame(char glyph) {
  auto buffer = CellBuffer::from({4, 1});
  auto text = GraphemeText::from(std::string(1, glyph));

  if (!buffer || !text || !buffer->put({0, 0}, *text, 0)) {
    throw std::runtime_error("invalid writer test setup");
  }

  return std::move(*buffer);
}

void writerIntegration() {
  FakeClock clock;
  TestTerminal terminal(clock, {4, 1}, 1);
  FrameWriter writer(terminal);

  check(writer.submit(frame('a')).has_value(), "writer accepts initial frame");
  check(writer.flush(TerminalDeadline::max()).status == TerminalStatus::PROGRESS && writer.committedFrame(),
        "writer commits only completed transport");

  const auto before = terminal.snapshot();

  check(before.cells.at(0).glyph == "a", "real writer reaches independent terminal state");
  check(writer.submit(frame('b')).has_value(), "writer accepts update");
  terminal.failAfter(1);
  check(writer.flush(TerminalDeadline::max()).error.has_value() && writer.poisoned() && !writer.committedFrame(),
        "injected partial error poisons FrameWriter and invalidates committed frame");
  check(terminal.snapshot().cells == before.cells, "partial escape cannot paint target frame");
  check(!writer.submit(frame('c')) && !terminal.submit("x"), "failed writer/transport cannot retry uncertain output");
}

void malformed() {
  for (const auto text : {"", "PORTALS-TRACE 2\n", "PORTALS-TRACE 1x\n"}) {
    rejects(
      [&] {
        (void)parseTrace(text);
      },
      "version");
  }

  for (const auto line :
       {"", "0 INPUT", "0 INPUT 0", "0 INPUT zz", "0 RESIZE 1", "0 RESIZE -1 1", "0 RESIZE 99999999999999999999 1",
        "0 FAIL_AFTER 99999999999999999999", "0 RESIZE 4096 4096", "0 TICK extra", "0 UNKNOWN", "-1 TICK"}) {
    rejects(
      [&] {
        (void)parseTrace("PORTALS-TRACE 1\n" + std::string(line) + "\n");
      },
      "line 2");
  }

  rejects(
    [] {
      (void)parseTrace("PORTALS-TRACE 1\n2 TICK\n1 TICK\n");
    },
    "backward");
  rejects(
    [] {
      (void)parseTrace(std::string(TraceLimits::BYTES + 1, 'x'));
    },
    "file limit");

  Trace too_many(TraceLimits::RECORDS + 1, {0, TraceKind::TICK, {}});

  rejects(
    [&] {
      (void)recordTrace(too_many);
    },
    "record limit");

  Trace snapshots(TraceLimits::SNAPSHOTS, {0, TraceKind::SNAPSHOT, {}});

  rejects(
    [&] {
      (void)replay(snapshots, 1, 1);
    },
    "snapshot limit");

  Trace events{{0, TraceKind::INPUT, std::string(TraceLimits::EVENTS + 1, 'x')}};

  rejects(
    [&] {
      (void)replay(events, 1, 1);
    },
    "event limit");
  rejects(
    [] {
      (void)replay({}, 0, 1);
    },
    "positive");

  Trace huge{{0, TraceKind::OUTPUT, std::string(TraceLimits::BYTES + 1, 'x')}};

  rejects(
    [&] {
      (void)recordTrace(huge);
    },
    "payload limit");

  Trace unexpected{{0, TraceKind::TICK, "x"}};

  rejects(
    [&] {
      (void)replay(unexpected, 1, 1);
    },
    "unexpected record payload");
  rejects(
    [] {
      VtOracle oracle(std::numeric_limits<int>::max(), 2);
    },
    "viewport limit");
  for (const auto bytes :
       {"\x1b[999999999999999H", "\x1b[99;1H", "\x1b[38;2;999;0;0m", "\x1b[?1H", "\xc0\xaf", "\xed\xa0\x80"}) {
    rejects(
      [&] {
        VtOracle oracle(4, 1);

        oracle.feed(bytes);
      },
      "oracle:");
  }

  rejects(
    [] {
      VtOracle oracle(4, 1);

      oracle.feed("\x1b[" + std::string(65, '0'));
    },
    "CSI limit");
  rejects(
    [] {
      VtOracle oracle(4, 1);

      oracle.feed("e");

      for (std::size_t i = 0; i < VtOracle::MAX_GLYPH_BYTES; ++i) {
        oracle.feed("\xcc\x81");
      }
    },
    "glyph byte limit");

  VtOracle oracle(4, 1);

  oracle.feed("\x1b[1;3H\xe7\x95\x8c");
  oracle.resize(3, 1);
  check(oracle.at(2, 0) == VtOracle::Pixel{}, "resize must clear a clipped wide glyph, not retain an orphan lead");
  rejects(
    [&] {
      (void)oracle.at(-1, 0);
    },
    "coordinate");
}
} // namespace

int main(int argc, char **argv) {
  try {
    if (argc != 2) {
      throw std::runtime_error("fixture directory argument required");
    }

    fixtures(argv[1]);
    clockAndTransport();
    writerIntegration();
    malformed();
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';

    return 1;
  }

  return failures == 0 ? 0 : 1;
}
