// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#pragma once

#include <portals/application.hpp>
#include <portals/chart.hpp>
#include <portals/controls.hpp>
#include <portals/virtual_table.hpp>

#include <array>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <string>
#include <type_traits>

namespace virtualConsumer {
using namespace portals;

inline constexpr std::uint64_t DATA_REVISION = 7;
inline constexpr std::size_t ROW_COUNT = 64;
inline constexpr CellSize INITIAL_SIZE{64, 14};
inline constexpr CellSize RESIZED_SIZE{81, 18};

inline void check(bool condition, const char *message) {
  if (!condition) {
    std::cerr << message << '\n';
    std::exit(EXIT_FAILURE);
  }
}

// Only setup and assertions terminate; scene preparation returns original typed errors.
template <class T, class E> T take(std::expected<T, E> result) {
  check(result.has_value(), "Virtual consumer setup or assertion failed");

  if constexpr (!std::is_void_v<T>) {
    return std::move(*result);
  }
}

inline GraphemeText text(std::string_view value) {
  return GraphemeText::from(value).value();
}

// Each completion owns immutable rows. No worker, network or second scheduler is needed.
class Prices final : public ModelSnapshot<Ohlc> {
public:
  explicit Prices(bool ready = true) : ModelSnapshot(DATA_REVISION, ROW_COUNT), ready_(ready) {
    for (std::size_t index = 0; index < rows_.size(); ++index) {
      const auto price = 100.0 + static_cast<double>(index % 9);

      rows_[index] = {{index + 1, 1}, {price, price + 2, price - 1, price + 1}};
    }
  }

  ModelRows<Ohlc> read(RowRange range) const override {
    if (!ready_) {
      return {RangeState::PENDING, {}, {}};
    }

    return {RangeState::READY, std::span(rows_).subspan(range.first, range.count), {}};
  }

private:
  const bool ready_;
  std::array<ModelRow<Ohlc>, ROW_COUNT> rows_{};
};

inline std::array<TableColumn<Ohlc>, 2> priceColumns() {
  return {{{{1, 1},
            text("Open"),
            9,
            [](const Ohlc &value) {
              return text(std::to_string(static_cast<int>(value.open)));
            }},
           {{2, 1}, text("Close"), 9, [](const Ohlc &value) {
              return text(std::to_string(static_cast<int>(value.close)));
            }}}};
}

// Finite logical-time transport: no native handles, windows, threads or wall-clock sleeps.
class ReplayTerminal final : public ApplicationTerminal {
public:
  explicit ReplayTerminal(std::size_t checkpoints = 3) : checkpoints_(checkpoints) {
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
    const auto nextTime = (next_ + 1) * CHECKPOINT_INTERVAL_MS;

    time_ = std::max(time_, std::min(target, nextTime));

    if (next_ < checkpoints_ && time_ >= nextTime) {
      if (checkpoint) {
        checkpoint(next_, result);
      }

      ++next_;
    } else {
      result.status = next_ == checkpoints_ ? TerminalStatus::END_OF_INPUT : TerminalStatus::DEADLINE;
    }

    return result;
  }

  std::expected<CellSize, TerminalError> size() const override {
    return INITIAL_SIZE;
  }

  std::expected<void, TerminalError> submit(std::string bytes) override {
    if (!pending_.empty()) {
      return std::unexpected(TerminalError{TerminalErrorKind::OUTPUT_PENDING});
    }

    pending_ = std::move(bytes);
    ++submissions;

    return {};
  }

  TerminalWrite flush(TerminalDeadline, std::stop_token) override {
    const auto count = pending_.size();

    output += pending_;
    pending_.clear();

    return {TerminalStatus::PROGRESS, count, 0, {}};
  }

  std::expected<void, TerminalError> close() noexcept override {
    closed = true;

    return {};
  }

  std::function<void(std::size_t, TerminalRead &)> checkpoint;
  std::string output;
  std::size_t submissions{};
  bool closed{};

private:
  static constexpr std::uint64_t CHECKPOINT_INTERVAL_MS = 200;
  std::size_t checkpoints_;
  std::size_t next_{};
  std::uint64_t time_{};
  std::string pending_;
};

inline void reportCapture(const ReplayTerminal &terminal) {
  check(terminal.closed && !terminal.output.empty(), "Replay did not render and close");
  std::cout << "nested OHLC table+chart revision=7\n";

  constexpr std::string_view HEX = "0123456789abcdef";

  for (const unsigned char byte : terminal.output) {
    std::cout << HEX[byte >> 4] << HEX[byte & 15];
  }

  std::cout << '\n';
}
} // namespace virtualConsumer
