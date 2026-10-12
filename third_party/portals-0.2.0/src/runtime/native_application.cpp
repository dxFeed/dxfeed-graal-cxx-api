// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include <portals/application.hpp>

#include <algorithm>
#include <array>
#include <cerrno>
#include <string_view>
#include <utility>

namespace portals {
namespace {
constexpr std::string_view SAVE_CURSOR = "\x1b"
                                         "7";
constexpr std::string_view ENTER_SCREEN = "\x1b[?1049h";
#ifdef _WIN32
constexpr std::string_view SAVE_VISIBILITY = "";
constexpr std::string_view LEAVE_SCREEN = "\x1b[0m\x1b[?1049l";
#else
constexpr std::string_view SAVE_VISIBILITY = "\x1b[?25s";
constexpr std::string_view LEAVE_SCREEN = "\x1b[0m\x1b[?1049l\x1b[?25r";
#endif
constexpr std::string_view CANCEL_SEQUENCE = "\x18";
constexpr std::string_view SAVE_TRACKING = "\x1b[?9;1000;1001;1002;1003s";
constexpr std::string_view RESTORE_TRACKING = "\x1b[?9;1000;1001;1002;1003r";
constexpr std::string_view ENABLE_MOUSE = "\x1b[?1005;1015;1016l\x1b[?1000;1006h";
constexpr std::array ENCODING_MODES{1005, 1006, 1015, 1016};
constexpr std::size_t MAX_QUERY_REPLY_BYTES = 16;
constexpr int MODE_SET = 1;
constexpr int MODE_RESET = 2;
constexpr int MODE_PERMANENTLY_SET = 3;

constexpr auto MAX_MODE_TIMEOUT =
  std::chrono::duration_cast<std::chrono::milliseconds>(TerminalDeadline::duration::max());

TerminalDeadline deadlineAfter(std::chrono::milliseconds timeout) {
  const auto now = std::chrono::steady_clock::now();
  const auto interval = std::chrono::duration_cast<TerminalDeadline::duration>(timeout);

  if (now > TerminalDeadline{} && TerminalDeadline::max().time_since_epoch() - now.time_since_epoch() < interval) {
    return TerminalDeadline::max();
  }

  return now + interval;
}

// Remove only complete replies for our four queries. Preserve unrelated input and
// an unfinished reply suffix, including bytes arriving before or between replies.
std::size_t collectReplies(std::string &bytes, std::array<int, ENCODING_MODES.size()> &states) {
  std::size_t cursor = 0;

  while ((cursor = bytes.find('\x1b', cursor)) != std::string::npos) {
    const std::string_view tail(bytes.data() + cursor, bytes.size() - cursor);
    bool consumed = false;

    for (std::size_t index = 0; index < ENCODING_MODES.size(); ++index) {
      const auto prefix = "\x1b[?" + std::to_string(ENCODING_MODES[index]) + ";";
      const auto length = prefix.size() + 3;

      if (tail.starts_with(prefix) && tail.size() >= length && tail[prefix.size()] >= '0' &&
          tail[prefix.size()] <= '4' && tail.substr(prefix.size() + 1, 2) == "$y") {
        states[index] = tail[prefix.size()] - '0';
        bytes.erase(cursor, length);
        consumed = true;

        break;
      }

      if (tail.size() < length) {
        for (char state = '0'; state <= '4'; ++state) {
          const auto reply = prefix + state + "$y";

          if (std::string_view(reply).starts_with(tail)) {
            return tail.size();
          }
        }
      }
    }

    if (!consumed) {
      ++cursor;
    }
  }

  return 0;
}

} // namespace

namespace detail {
class NativeTerminal final : public ApplicationTerminal {
public:
  NativeTerminal(TerminalSession session, TerminalOptions options, std::chrono::milliseconds timeout, bool mouse,
                 NativeScreenMode screen)
      : session_(std::move(session)), inputLimit_(options.max_input_bytes), outputLimit_(options.max_output_bytes),
        timeout_(timeout), mouseReporting_(mouse), screen_(screen) {
    if (screen_ == NativeScreenMode::ALTERNATE_SCREEN) {
      screenCleanup_ = std::string(CANCEL_SEQUENCE) + std::string(LEAVE_SCREEN);
      screenSave_ = std::string(SAVE_VISIBILITY) + std::string(SAVE_CURSOR);
      captureCleanup_ = std::string(CANCEL_SEQUENCE);
    }
  }

  ~NativeTerminal() noexcept override {
    (void)close();
  }

  std::expected<void, TerminalError> start() {
    if (screen_ == NativeScreenMode::ALTERNATE_SCREEN) {
      auto saved = session_.saveCursorVisibility();

      if (!saved) {
        return saved;
      }
    }

    auto mouse = startMouse();

    if (!mouse || screen_ == NativeScreenMode::MAIN_SCREEN) {
      started_ = mouse.has_value();

      return mouse;
    }

    const auto deadline = deadlineAfter(timeout_);

    if (!screenSave_.empty()) {
      // Confirm cursor/rendition capture separately: reset 1049 may restore this cache even on the main screen.
      captureStarted_ = true;

      auto captured = send(screenSave_, deadline);

      if (!captured) {
        return captured;
      }
    }

    // Arm rollback before submission: a failed write may already have entered the alternate screen.
    screenRestoreNeeded_ = true;

    auto entered = send(ENTER_SCREEN, deadline);

    started_ = entered.has_value();

    return entered;
  }

  std::expected<void, TerminalError> startMouse() {
#ifndef _WIN32
    if (!mouseReporting_) {
      return {};
    }

    const auto deadline = deadlineAfter(timeout_);
    auto queried = queryEncodings(deadline);

    if (!queried) {
      return queried;
    }

    // xterm 390 restores encoding with an 8-bit Boolean cast (DoRM), truncating
    // the saved numeric encoding. Use queried values; tracking uses integer DoRM0.
    restoreBytes_ += RESTORE_TRACKING;

    // Reject before changing modes if the mandatory cleanup batch cannot fit.
    if (CANCEL_SEQUENCE.size() + restoreBytes_.size() + screenCleanup_.size() > outputLimit_) {
      return std::unexpected(TerminalError{TerminalErrorKind::OUTPUT_LIMIT});
    }

    mouseCleanup_ = std::string(CANCEL_SEQUENCE) + restoreBytes_;
    combinedCleanup_ = mouseCleanup_ + (screenCleanup_.empty() ? std::string{} : screenCleanup_.substr(1));

    auto saved = send(SAVE_TRACKING, deadline);

    if (!saved) {
      return saved;
    }

    // Arm cleanup before allocating submission storage or partially changing modes.
    restoreNeeded_ = true;

    return send(ENABLE_MOUSE, deadline);
#else
    return {};
#endif
  }

  TerminalRead read(TerminalDeadline deadline, std::stop_token stop) override {
    if (!closed_ && (!startupInput_.bytes.empty() || startupInput_.resized)) {
      if (stop.stop_requested()) {
        return {TerminalStatus::STOPPED, {}, {}, {}, {}};
      }

      if (std::chrono::steady_clock::now() >= deadline) {
        return {TerminalStatus::DEADLINE, {}, {}, {}, {}};
      }

      return std::exchange(startupInput_, {});
    }

    return session_.read(deadline, stop);
  }

  std::expected<CellSize, TerminalError> size() const override {
    return session_.size();
  }

  std::expected<void, TerminalError> submit(std::string bytes) override {
    return session_.submit(std::move(bytes));
  }

  TerminalWrite flush(TerminalDeadline deadline, std::stop_token stop) override {
    return session_.flush(deadline, stop);
  }

  std::expected<void, TerminalError> close() noexcept override {
    if (closed_) {
      return {};
    }

    closed_ = true;
    std::optional<TerminalError> failure;

    const auto deadline = deadlineAfter(timeout_);
    std::string cleanup;

    if (restoreNeeded_ || screenRestoreNeeded_ || captureStarted_) {
      if (started_) {
        try {
          auto drained = finishOutput(deadline);

          if (!drained) {
            failure = drained.error();
          }
        } catch (...) {
          failure = TerminalError{TerminalErrorKind::SYSTEM, EIO};
        }
      }

      // Never complete a failed startup sequence while trying to roll it back.

      if (restoreNeeded_ && screenRestoreNeeded_) {
        cleanup = std::move(combinedCleanup_);
      } else if (restoreNeeded_) {
        cleanup = std::move(mouseCleanup_);
      } else if (screenRestoreNeeded_) {
        cleanup = std::move(screenCleanup_);
      } else {
        cleanup = std::move(captureCleanup_);
      }
    }

    if (!cleanup.empty()) {
      // Joining uncertain native I/O may outlive the drain deadline. Start the
      // mandatory restoration budget only after no worker borrows the old batch.
      auto joined = session_.joinOutputWriter();

      if (!joined) {
        if (failure) {
          failure->restoration_code = joined.error().native_code;
        } else {
          failure = joined.error();
        }
      }
    }

    auto closed =
      cleanup.empty() ? session_.close() : session_.closeWithOutput(std::move(cleanup), deadlineAfter(timeout_));

    if (!closed) {
      if (failure) {
        failure->restoration_code = closed.error().native_code;
      } else {
        failure = closed.error();
      }
    }

    if (failure) {
      return std::unexpected(*failure);
    }

    return {};
  }

private:
  std::expected<void, TerminalError> queryEncodings(TerminalDeadline deadline) {
    std::string queries;

    for (const int mode : ENCODING_MODES) {
      queries += "\x1b[?" + std::to_string(mode) + "$p";
    }

    auto sent = send(queries, deadline);

    if (!sent) {
      return sent;
    }

    std::array<int, ENCODING_MODES.size()> states;

    states.fill(-1);

    for (;;) {
      auto input = session_.read(deadline);

      if (input.error) {
        return std::unexpected(*input.error);
      }

      if (input.status != TerminalStatus::PROGRESS) {
        return std::unexpected(TerminalError{TerminalErrorKind::SYSTEM, ETIMEDOUT});
      }

      if (input.resized) {
        startupInput_.resized = input.resized;
      }

      std::size_t offset = 0;

      while (offset < input.bytes.size()) {
        const auto room = inputLimit_ + MAX_QUERY_REPLY_BYTES - startupInput_.bytes.size();
        const auto amount = std::min(room, input.bytes.size() - offset);

        if (amount == 0) {
          return std::unexpected(TerminalError{TerminalErrorKind::INPUT_LIMIT});
        }

        startupInput_.bytes.append(input.bytes, offset, amount);
        offset += amount;

        const auto suffix = collectReplies(startupInput_.bytes, states);

        if (startupInput_.bytes.size() - suffix > inputLimit_) {
          return std::unexpected(TerminalError{TerminalErrorKind::INPUT_LIMIT});
        }
      }

      if (std::ranges::all_of(states, [](int state) {
            return state >= 0;
          })) {
        break;
      }
    }

    if (startupInput_.bytes.size() > inputLimit_) {
      return std::unexpected(TerminalError{TerminalErrorKind::INPUT_LIMIT});
    }

    const auto sgr = states[1];

    if ((sgr != MODE_SET && sgr != MODE_RESET && sgr != MODE_PERMANENTLY_SET) || states[0] == MODE_PERMANENTLY_SET ||
        states[2] == MODE_PERMANENTLY_SET || states[3] == MODE_PERMANENTLY_SET) {
      return std::unexpected(TerminalError{TerminalErrorKind::UNSUPPORTED});
    }

    for (std::size_t index = 0; index < states.size(); ++index) {
      if (states[index] == MODE_SET || states[index] == MODE_RESET) {
        restoreBytes_ += "\x1b[?" + std::to_string(ENCODING_MODES[index]) + "l";
      }
    }

    for (std::size_t index = 0; index < states.size(); ++index) {
      if (states[index] == MODE_SET) {
        restoreBytes_ += "\x1b[?" + std::to_string(ENCODING_MODES[index]) + "h";
      }
    }

    return {};
  }

  std::expected<void, TerminalError> finishOutput(TerminalDeadline deadline) {
    const auto result = session_.flush(deadline);

    if (result.error) {
      return std::unexpected(*result.error);
    }

    if (result.status != TerminalStatus::PROGRESS || result.pending != 0) {
      return std::unexpected(TerminalError{TerminalErrorKind::SYSTEM, ETIMEDOUT});
    }

    return {};
  }

  std::expected<void, TerminalError> send(std::string_view bytes, TerminalDeadline deadline) {
    auto accepted = session_.submit(std::string(bytes));

    if (!accepted) {
      return accepted;
    }

    return finishOutput(deadline);
  }

  TerminalSession session_;
  std::size_t inputLimit_;
  std::size_t outputLimit_;
  std::chrono::milliseconds timeout_;
  TerminalRead startupInput_;
  std::string restoreBytes_, mouseCleanup_, screenCleanup_, combinedCleanup_, screenSave_, captureCleanup_;
  bool mouseReporting_{}, restoreNeeded_{}, captureStarted_{}, screenRestoreNeeded_{}, started_{}, closed_{};
  NativeScreenMode screen_;
};
} // namespace detail

std::expected<std::unique_ptr<ApplicationTerminal>, TerminalError>
openNativeApplicationTerminal(NativeTerminalHandles handles, TerminalOptions options,
                              std::chrono::milliseconds modeTimeout, bool mouseReporting, NativeScreenMode screen) {
  if (modeTimeout.count() <= 0 || modeTimeout > MAX_MODE_TIMEOUT ||
      options.max_input_bytes > std::string{}.max_size() - MAX_QUERY_REPLY_BYTES) {
    return std::unexpected(TerminalError{TerminalErrorKind::INVALID_OPTIONS});
  }

  if (screen != NativeScreenMode::MAIN_SCREEN && screen != NativeScreenMode::ALTERNATE_SCREEN) {
    return std::unexpected(TerminalError{TerminalErrorKind::INVALID_OPTIONS});
  }

  if (screen == NativeScreenMode::ALTERNATE_SCREEN &&
      (CANCEL_SEQUENCE.size() + LEAVE_SCREEN.size() > options.max_output_bytes ||
       SAVE_VISIBILITY.size() + SAVE_CURSOR.size() > options.max_output_bytes ||
       ENTER_SCREEN.size() > options.max_output_bytes)) {
    return std::unexpected(TerminalError{TerminalErrorKind::OUTPUT_LIMIT});
  }

  auto session = TerminalSession::open(handles, options);

  if (!session) {
    return std::unexpected(session.error());
  }

  auto terminal =
    std::make_unique<detail::NativeTerminal>(std::move(*session), options, modeTimeout, mouseReporting, screen);
  auto started = terminal->start();

  if (!started) {
    auto error = started.error();
    auto cleanup = terminal->close();

    if (!cleanup) {
      error.restoration_code = cleanup.error().native_code;
    }

    return std::unexpected(error);
  }

  return terminal;
}
} // namespace portals
