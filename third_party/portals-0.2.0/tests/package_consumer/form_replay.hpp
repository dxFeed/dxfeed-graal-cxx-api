// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#pragma once
#include "form_support.hpp"

namespace consumer {
inline KeyEvent key(InputKey value, InputModifiers modifiers = {}) {
  return {value, {}, modifiers, KeyAction::PRESS};
}

inline std::vector<std::vector<InputEvent>> formScript() {
  const KeyEvent selectAll{InputKey::CHARACTER, U'a', {.control = true}, KeyAction::PRESS};

  return {{selectAll, PasteInputEvent{"Ada"}},
          {key(InputKey::ENTER)},
          {key(InputKey::TAB), TextInputEvent{U' '}},
          {key(InputKey::TAB), key(InputKey::ENTER)},
          {key(InputKey::TAB), key(InputKey::RIGHT)},
          {key(InputKey::F2)},
          {key(InputKey::TAB), selectAll, key(InputKey::BACKSPACE), key(InputKey::ENTER)},
          {key(InputKey::ESCAPE)},
          {TextInputEvent{U'!'}},
          {key(InputKey::F3)},
          {key(InputKey::ENTER)},
          {key(InputKey::ESCAPE)},
          {PasteInputEvent{std::string(1, static_cast<char>(0xff))}},
          {key(InputKey::TAB, {.shift = true}), key(InputKey::DELETE_KEY)},
          {key(InputKey::F10)}};
}

// No native transport is opened. Logical time advances to finite input and frame deadlines.
class ReplayTerminal final : public ApplicationTerminal {
public:
  explicit ReplayTerminal(std::vector<std::vector<InputEvent>> script = formScript()) : script_(std::move(script)) {
  }

  SchedulerClock clock() noexcept {
    return {[](void *context) noexcept {
              return static_cast<ReplayTerminal *>(context)->time_;
            },
            this};
  }

  TerminalRead read(TerminalDeadline deadline, std::stop_token stop) override {
    TerminalRead result;

    if (stop.stop_requested()) {
      result.status = TerminalStatus::STOPPED;

      return result;
    }

    const auto target = static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(deadline.time_since_epoch()).count());
    const auto nextTime = (next_ + 1) * EVENT_INTERVAL_MS;

    time_ = std::max(time_, std::min(target, nextTime));

    if (next_ < script_.size() && time_ >= nextTime) {
      if (checkpoint) {
        checkpoint(next_);
      }

      result.events = script_[next_++];
    } else {
      result.status = next_ == script_.size() ? TerminalStatus::END_OF_INPUT : TerminalStatus::DEADLINE;
    }

    return result;
  }

  std::expected<CellSize, TerminalError> size() const override {
    return CellSize{64, 12};
  }

  std::expected<void, TerminalError> submit(std::string bytes) override {
    if (!pending_.empty()) {
      return std::unexpected(TerminalError{TerminalErrorKind::OUTPUT_PENDING});
    }

    pending_ = std::move(bytes);

    return {};
  }

  TerminalWrite flush(TerminalDeadline, std::stop_token) override {
    const auto count = pending_.size();

    if (output.size() + count > MAX_CAPTURE_BYTES) {
      return {TerminalStatus::FAILED, 0, count, TerminalError{TerminalErrorKind::OUTPUT_LIMIT}};
    }

    output += pending_;
    pending_.clear();

    return {TerminalStatus::PROGRESS, count, 0, {}};
  }

  std::expected<void, TerminalError> close() noexcept override {
    closed = true;

    return {};
  }

  std::function<void(std::size_t)> checkpoint;
  std::string output;
  bool closed{};

private:
  static constexpr std::uint64_t EVENT_INTERVAL_MS = 100;
  static constexpr std::size_t MAX_CAPTURE_BYTES = 1024 * 1024;
  std::vector<std::vector<InputEvent>> script_;
  std::string pending_;
  std::uint64_t time_{};
  std::size_t next_{};
};

inline ApplicationOptions formOptions() {
  ApplicationOptions options;

  options.scheduler.data_capacity = 1;

  return options;
}

// Hex output is exact and safe in CTest logs; comparison does not depend on a lossy screen digest.
inline void reportCapture(const ReplayTerminal &terminal, const Outcome &outcome) {
  check(terminal.closed && !terminal.output.empty(), "Application did not render and close replay transport");
  check(outcome.commits == 1 && outcome.runActions == 1 && outcome.checkboxActions == 1,
        "Business callback count changed during synchronization");
  std::cout << "name=Remote alerts=false level=55 commits=1 run=1 checkbox=1\n";

  constexpr std::string_view HEX = "0123456789abcdef";

  for (const unsigned char byte : terminal.output) {
    std::cout << HEX[byte >> 4] << HEX[byte & 15];
  }

  std::cout << '\n';
}
} // namespace consumer
