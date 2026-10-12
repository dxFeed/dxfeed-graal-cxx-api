// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#pragma once
#include <algorithm>
#include <portals/application.hpp>

namespace example {
// A deterministic input source uses the same Application entry point as a native session.
// Clock and terminal share an epoch; output is captured only up to an explicit byte budget.
class ReplayTerminal : public portals::ApplicationTerminal {
public:
  struct Input {
    std::uint64_t at;
    std::string bytes;
    std::optional<portals::CellSize> resize;
  };

  explicit ReplayTerminal(std::vector<Input> input = {}, std::uint64_t end = 1000, portals::CellSize size = {80, 24})
      : input_(std::move(input)), end_(end), size_(size) {
  }

  portals::SchedulerClock clock() noexcept {
    return {[](void *context) noexcept {
              return static_cast<ReplayTerminal *>(context)->time_;
            },
            this};
  }

  portals::TerminalRead read(portals::TerminalDeadline deadline, std::stop_token stop) override {
    portals::TerminalRead result;

    if (stop.stop_requested()) {
      result.status = portals::TerminalStatus::STOPPED;

      return result;
    }

    auto target = static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(deadline.time_since_epoch()).count());

    if (target <= time_) {
      result.status = portals::TerminalStatus::DEADLINE;

      return result;
    }

    time_ = std::min(target, end_);

    if (next_ < input_.size() && input_[next_].at <= time_) {
      time_ = std::max(input_[next_].at, previous_);
      result.bytes = input_[next_].bytes;
      result.resized = input_[next_++].resize;

      if (result.resized) {
        size_ = *result.resized;
      }
    } else {
      result.status = time_ >= end_ ? portals::TerminalStatus::END_OF_INPUT : portals::TerminalStatus::DEADLINE;
    }

    previous_ = time_;

    return result;
  }

  std::expected<portals::CellSize, portals::TerminalError> size() const override {
    return size_;
  }

  std::expected<void, portals::TerminalError> submit(std::string bytes) override {
    if (!pending_.empty()) {
      return std::unexpected(portals::TerminalError{portals::TerminalErrorKind::OUTPUT_PENDING});
    }

    pending_ = std::move(bytes);

    return {};
  }

  portals::TerminalWrite flush(portals::TerminalDeadline, std::stop_token) override {
    const auto count = pending_.size();

    if (output.size() + count > MAX_CAPTURE) {
      return {portals::TerminalStatus::FAILED, 0, count,
              portals::TerminalError{portals::TerminalErrorKind::OUTPUT_LIMIT}};
    }

    output += pending_;
    pending_.clear();

    return {portals::TerminalStatus::PROGRESS, count, 0, {}};
  }

  std::expected<void, portals::TerminalError> close() noexcept override {
    closed = true;

    return {};
  }

  std::string output;
  bool closed{};

private:
  static constexpr std::size_t MAX_CAPTURE = 4 * 1024 * 1024;
  std::vector<Input> input_;
  std::uint64_t end_, time_{}, previous_{};
  portals::CellSize size_;
  std::size_t next_{};
  std::string pending_;
};
} // namespace example
