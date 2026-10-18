// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include "support/vt_oracle.hpp"
#include <portals/diff.hpp>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <utility>
#ifdef __unix__
#  include <fcntl.h>
#  include <sys/ioctl.h>
#  include <termios.h>
#  include <unistd.h>
#endif

namespace {
using namespace portals;
using namespace std::chrono_literals;
int failures = 0;

void check(bool value, const char *message) {
  if (!value) {
    std::cerr << message << '\n';
    ++failures;
  }
}

std::string utf8(std::u8string_view value) {
  return {reinterpret_cast<const char *>(value.data()), value.size()};
}

CellBuffer frame(bool changed) {
  auto result = CellBuffer::from({8, 2}, {}, {16, 8, 128, 32, 256, 8});
  const CellStyle style{{RgbColor{1, 127, 255}}, {RgbColor{70, 60, 50}}, true, true, true};
  auto glyph = GraphemeText::from(utf8(changed ? u8"😀" : u8"界"));
  auto accent = GraphemeText::from(utf8(u8"é"));

  if (!result || !result->put({changed ? 6 : 4, 1}, *glyph, 0, style) ||
      !result->put({1, 0}, *accent, 0, changed ? style : CellStyle{})) {
    std::exit(2);
  }

  return std::move(*result);
}

class TestOutput final : public FrameOutput {
public:
  explicit TestOutput(test_support::VtOracle &terminal) : terminal(terminal) {
  }

  std::expected<void, TerminalError> submit(std::string value) override {
    ++submissions;

    if (reject) {
      return std::unexpected(TerminalError{TerminalErrorKind::OUTPUT_PENDING});
    }

    if (!bytes.empty()) {
      std::exit(2);
    }

    bytes = std::move(value);

    return {};
  }

  TerminalWrite flush(TerminalDeadline, std::stop_token stop) override {
    ++flushes;

    if (throw_error) {
      throw std::runtime_error("injected transport exception");
    }

    if (stop.stop_requested()) {
      return {TerminalStatus::STOPPED, 0, bytes.size(), {}};
    }

    const auto count = std::min(limit, bytes.size());

    terminal.feed(std::string_view(bytes).substr(0, count));
    bytes.erase(0, count);

    if (error) {
      // Unknown native effects: feed count bytes but report zero completed.
      return {TerminalStatus::FAILED, 0, bytes.size(), TerminalError{TerminalErrorKind::SYSTEM, 995}};
    }

    if (uncertain_deadline) {
      return {TerminalStatus::DEADLINE, 0, bytes.size(), TerminalError{TerminalErrorKind::SYSTEM, 995}};
    }

    if (lie) {
      return {TerminalStatus::PROGRESS, count + 1, bytes.size(), {}};
    }

    return {bytes.empty() ? TerminalStatus::PROGRESS : TerminalStatus::DEADLINE, count, bytes.size(), {}};
  }

  void close() {
    bytes.clear();
  } // Quiescence; deliberately preserves the VT parser and screen.

  test_support::VtOracle &terminal;
  std::size_t limit{std::numeric_limits<std::size_t>::max()};
  int submissions{};
  int flushes{};
  bool error{};
  bool reject{};
  bool lie{};
  bool uncertain_deadline{};
  bool throw_error{};
  std::string bytes;
};

void allSplits() {
  auto old = frame(false);
  auto target = frame(true);
  const auto encoded = *encodeDiff(&old, target);
  const auto deadline = TerminalDeadline::max();

  for (std::size_t split = 0; split <= encoded.size(); ++split) {
    test_support::VtOracle terminal(8, 2);
    TestOutput output(terminal);
    FrameWriter writer(output);

    check(writer.submit(frame(false)).has_value(), "accept initial owning frame");
    check(output.submissions == 0 && output.flushes == 0, "submit performs no transport operation");
    check(writer.flush(deadline).status == TerminalStatus::PROGRESS, "complete initial frame");

    const auto *previous = writer.committedFrame();

    check(writer.submit(frame(true)).has_value(), "accept changed frame");
    check(writer.committedFrame() == previous, "calculation alone never commits desired frame");
    output.limit = split;

    const auto partial = writer.flush(deadline);

    if (split < encoded.size()) {
      check(partial.status == TerminalStatus::DEADLINE && writer.pending(), "every prefix remains pending");
      check(writer.committedFrame() == previous, "every escape/UTF-8 split preserves previous commit");

      auto rejected = frame(false);

      check(writer.submit(std::move(rejected)).error() == FrameError::PENDING && rejected.size() == CellSize{8, 2},
            "busy rejects new frame without consuming it");

      std::stop_source stop;

      stop.request_stop();
      check(writer.flush(deadline, stop.get_token()).status == TerminalStatus::STOPPED &&
              writer.committedFrame() == previous,
            "stop retains transaction without false commit");
      output.limit = std::numeric_limits<std::size_t>::max();
      check(writer.flush(deadline).status == TerminalStatus::PROGRESS, "resume exact suffix once");
    }

    check(!writer.pending() && !writer.poisoned() && writer.committedFrame() != nullptr,
          "complete transaction alone commits frame");
    check(terminal.equals(target) && terminal.neutral() && terminal.scrolls() == 0,
          "every short-write split yields exact screen, styles, width, cursor");
    check(output.submissions == 2, "partial resume never resubmits an escape/UTF-8 prefix");
    check(writer.submit(frame(true)).has_value() && !writer.pending(), "unchanged frame commits without transport");
    check(writer.flush(deadline).status == TerminalStatus::PROGRESS && output.submissions == 2,
          "unchanged frame emits zero updates");
  }

  for (std::size_t split = 0; split <= encoded.size(); ++split) {
    test_support::VtOracle terminal(8, 2);
    TestOutput output(terminal);
    FrameWriter writer(output);

    check(writer.submit(frame(false)).has_value(), "error fixture initial submission");
    (void)writer.flush(deadline);
    check(writer.submit(frame(true)).has_value(), "error fixture changed submission");
    output.limit = split;
    output.error = true;

    const auto failed = writer.flush(deadline);

    check(failed.status == TerminalStatus::FAILED && writer.poisoned() && !writer.committedFrame(),
          "every unknown error prefix invalidates belief despite reported zero bytes");
    check(writer.submit(frame(false)).error() == FrameError::POISONED, "uncertain writer cannot emit another frame");

    const auto calls = output.flushes;

    (void)writer.flush(deadline);
    check(output.flushes == calls, "poisoned output is never retried");
    output.close();

    TestOutput reopened(terminal);
    FrameWriter recovered(reopened);

    check(recovered.submit(frame(false)).has_value(), "new owner starts a full repaint after quiescent close");
    check(recovered.flush(deadline).status == TerminalStatus::PROGRESS, "CAN baseline completes recovery");
    check(terminal.equals(old) && terminal.neutral(), "CAN recovers every own CSI/UTF-8 prefix without oracle reset");
  }
}

void initialFrameSplits() {
  const auto desired = frame(true);
  const auto initial_bytes = *encodeDiff(nullptr, desired);

  for (std::size_t split = 0; split <= initial_bytes.size(); ++split) {
    for (bool error : {false, true}) {
      test_support::VtOracle terminal(8, 2);
      TestOutput output(terminal);
      FrameWriter writer(output);

      check(writer.submit(frame(true)).has_value(), "initial split fixture accepts frame");
      output.limit = split;
      output.error = error;

      const auto first = writer.flush(TerminalDeadline::max());

      if (error) {
        check(first.status == TerminalStatus::FAILED && writer.poisoned() && !writer.committedFrame(),
              "initial unknown output never commits a belief");
        output.close();

        TestOutput fresh(terminal);
        FrameWriter recovered(fresh);

        check(recovered.submit(frame(true)).has_value(), "initial prefix recovery accepts full frame");
        (void)recovered.flush(TerminalDeadline::max());
      } else {
        if (split < initial_bytes.size()) {
          check(writer.pending() && !writer.committedFrame(), "every initial byte split preserves unknown belief");
          output.limit = std::numeric_limits<std::size_t>::max();
          (void)writer.flush(TerminalDeadline::max());
        }

        check(!writer.pending() && writer.committedFrame(), "initial frame commits only on completion");
      }

      check(terminal.equals(desired) && terminal.neutral() && terminal.scrolls() == 0,
            "every initial prefix resumes or safely recovers full frame");
    }
  }
}

void limitsAndErrors() {
  const auto deadline = TerminalDeadline::max();
  test_support::VtOracle terminal(8, 2);
  TestOutput output(terminal);
  FrameWriter small(output, {1, 1024 * 1024});
  auto preserved = frame(false);

  check(small.submit(std::move(preserved)).error() == FrameError::OUTPUT_LIMIT && preserved.size() == CellSize{8, 2},
        "byte limit rejection retains caller ownership");

  FrameWriter memory(output, {1024 * 1024, 1});

  check(memory.submit(std::move(preserved)).error() == FrameError::FRAME_LIMIT && !memory.pending(),
        "frame storage cap bounds owned snapshots");
  for (int fault = 0; fault < 4; ++fault) {
    TestOutput broken(terminal);
    FrameWriter writer(broken);

    broken.reject = fault == 0;
    broken.lie = fault == 1;
    broken.throw_error = fault == 2;
    broken.uncertain_deadline = fault == 3;
    check(writer.submit(frame(false)).has_value(), "fault fixture accepts frame");

    bool caught = false;

    try {
      const auto result = writer.flush(deadline);

      if (fault == 3) {
        check(result.status == TerminalStatus::FAILED && result.error && result.error->native_code == 995,
              "uncertain deadline preserves native error and cannot resume");
      }
    } catch (const std::runtime_error &) {
      caught = true;
    }

    check(writer.poisoned() && !writer.committedFrame(), "rejection/inconsistent accounting/exception fail closed");
    check(caught == (fault == 2), "transport exception propagates after poisoning");
  }
}
#ifdef __unix__
class Fd {
public:
  explicit Fd(int value) : value(value) {
    if (value < 0) {
      throw std::runtime_error("PTY fd");
    }
  }

  ~Fd() {
    if (value >= 0) {
      ::close(value);
    }
  }

  Fd(const Fd &) = delete;
  Fd &operator=(const Fd &) = delete;
  int value;
};

void nativePty() {
  Fd master(posix_openpt(O_RDWR | O_NOCTTY | O_NONBLOCK));

  if (grantpt(master.value) != 0 || unlockpt(master.value) != 0) {
    throw std::runtime_error("PTY grant");
  }

  Fd slave(open(ptsname(master.value), O_RDWR | O_NOCTTY));
  termios before{};

  if (tcgetattr(slave.value, &before) != 0) {
    throw std::runtime_error("PTY termios");
  }

  winsize size{80, 160, 0, 0};

  if (ioctl(slave.value, TIOCSWINSZ, &size) != 0) {
    throw std::runtime_error("PTY size");
  }

  auto native = TerminalSession::open({slave.value, slave.value});

  if (!native) {
    throw std::runtime_error("PTY session");
  }

  auto desired = CellBuffer::from({160, 80});
  auto glyph = GraphemeText::from("x");
  const CellStyle rich{{RgbColor{255, 1, 2}}, {RgbColor{3, 4, 5}}, true, true, true};

  for (int y = 0; y < 80; ++y) {
    for (int x = 0; x < 160; ++x) {
      if (!desired->put({x, y}, *glyph, 0, x % 2 ? rich : CellStyle{})) {
        std::exit(2);
      }
    }
  }

  const auto expected = *encodeDiff(nullptr, *desired);
  TerminalFrameOutput output(*native);
  FrameWriter writer(output);

  check(writer.submit(std::move(*desired)).has_value(), "native writer accepts bounded pressure frame");

  auto progress = writer.flush(std::chrono::steady_clock::now() + 20ms);

  check(progress.status == TerminalStatus::DEADLINE && progress.completed > 0 && progress.pending > 0 &&
          !writer.committedFrame(),
        "real PTY pressure returns partial output without belief commit");

  std::stop_source stop;

  stop.request_stop();
  check(writer.flush(TerminalDeadline::max(), stop.get_token()).status == TerminalStatus::STOPPED,
        "native stopped partial write preserves suffix");

  std::string received;
  const auto drain = [&] {
    char bytes[8192];

    for (;;) {
      const auto count = ::read(master.value, bytes, sizeof(bytes));

      if (count <= 0) {
        break;
      }

      received.append(bytes, static_cast<std::size_t>(count));
    }
  };
  const auto deadline = std::chrono::steady_clock::now() + 5s;

  while (writer.pending() && !writer.poisoned() && std::chrono::steady_clock::now() < deadline) {
    drain();
    progress = writer.flush(std::chrono::steady_clock::now() + 1ms);
  }

  drain();
  check(!writer.pending() && !writer.poisoned() && writer.committedFrame(), "real PTY resumed transaction completes");
  check(received == expected, "real PTY sends exactly one batch, no dropped or repeated suffix");

  test_support::VtOracle oracle(160, 80);

  oracle.feed(received);
  check(writer.committedFrame() && oracle.equals(*writer.committedFrame()) && oracle.neutral(),
        "native PTY bytes reproduce committed frame through independent oracle");
  check(native->close().has_value(), "native session closes after owned output completes");

  termios after{};

  check(tcgetattr(slave.value, &after) == 0 && before.c_iflag == after.c_iflag && before.c_oflag == after.c_oflag &&
          before.c_cflag == after.c_cflag && before.c_lflag == after.c_lflag &&
          std::equal(std::begin(before.c_cc), std::end(before.c_cc), std::begin(after.c_cc)),
        "native modes restored");

  auto disconnected = TerminalSession::open({slave.value, slave.value});

  if (!disconnected) {
    throw std::runtime_error("PTY error session");
  }

  TerminalFrameOutput lost_output(*disconnected);
  FrameWriter lost(lost_output);

  check(lost.submit(frame(true)).has_value(), "native error fixture accepts frame");
  ::close(master.value);
  master.value = -1;

  const auto failed = lost.flush(std::chrono::steady_clock::now() + 100ms);

  check(failed.status == TerminalStatus::FAILED && lost.poisoned() && !lost.committedFrame(),
        "real PTY hangup poisons writer and invalidates belief");
  (void)disconnected->close(); // A vanished terminal can also make mode restoration fail.
}
#endif
} // namespace

int main() {
  allSplits();
  initialFrameSplits();
  limitsAndErrors();
#ifdef __unix__
  nativePty();
#endif
  return failures == 0 ? 0 : 1;
}
