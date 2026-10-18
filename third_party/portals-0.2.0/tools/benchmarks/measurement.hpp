// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#pragma once
#include "../../examples/dashboard/dashboard.hpp"
#include <algorithm>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace benchmark {
using namespace portals;

/** @brief Percentiles exist only for a complete, nonempty bounded sample set. */
struct Summary {
  std::optional<std::uint64_t> p50, p95, maximum;
};

/** @brief First-N samples with explicit overflow; never allocates in add(). */
class Samples {
public:
  /** @param capacity Maximum retained samples, reserved before running. */
  explicit Samples(std::size_t capacity) : capacity_(capacity) {
    values_.reserve(capacity);
  }

  /** @param value Duration in the caller's named units.
   * @param inWindow False excludes warmup without counting overflow. */
  void add(std::uint64_t value, bool inWindow) {
    if (!inWindow) {
      return;
    }

    if (values_.size() == capacity_) {
      ++overflow_;

      return;
    }

    values_.push_back(value);
  }

  /** @return Owning retained samples, in delivery order. */
  const std::vector<std::uint64_t> &values() const noexcept {
    return values_;
  }

  /** @return Number of samples which could not be retained. */
  std::uint64_t overflow() const noexcept {
    return overflow_;
  }

  /** @return Nearest-rank percentiles, or N/A for empty/incomplete samples.
   * Sorting and copying happen only after Application::run. */
  Summary summary() const {
    if (values_.empty() || overflow_) {
      return {};
    }

    auto sorted = values_;

    std::ranges::sort(sorted);

    return {sorted[(sorted.size() * 50 + 99) / 100 - 1], sorted[(sorted.size() * 95 + 99) / 100 - 1], sorted.back()};
  }

private:
  std::size_t capacity_;
  std::uint64_t overflow_{};
  std::vector<std::uint64_t> values_;
};

/** @brief Transport totals, including incomplete/uncertain flushes exactly once. */
struct OutputCounters {
  std::uint64_t submitted{}, encoded{}, flushes{}, confirmed{}, completedFrames{}, partial{}, uncertain{};
  std::size_t pending{};
  bool valid{true};

  /** @return Whether accepted output has a known complete transport state. */
  bool complete() const noexcept {
    return valid && pending == 0;
  }

  /** @param bytes Encoded bytes accepted by the underlying transport. */
  void accept(std::size_t bytes) noexcept {
    ++submitted;
    encoded += bytes;
    pending = bytes;
  }

  /** @param result Exact result of this flush, never a cumulative byte count. */
  void flush(const TerminalWrite &result) noexcept {
    ++flushes;
    confirmed += result.completed;

    const bool consistent = result.completed <= pending && result.pending == pending - result.completed;

    if (!consistent || result.error) {
      valid = false;
      ++uncertain;
    }

    if (result.pending) {
      ++partial;
    } else if (valid && result.status == TerminalStatus::PROGRESS) {
      ++completedFrames;
    }

    pending = result.pending;
  }
};

/** @brief Real-clock headless transport; input waits until the actual deadline. */
class HeadlessTerminal final : public ApplicationTerminal {
public:
  /** @param size Logical viewport; this is explicitly not a native emulator. */
  explicit HeadlessTerminal(CellSize size = {120, 40}) : size_(size) {
  }

  TerminalRead read(TerminalDeadline deadline, std::stop_token stop) override {
    std::unique_lock lock(mutex_);

    condition_.wait_until(lock, stop, deadline, [] {
      return false;
    });

    return {stop.stop_requested() ? TerminalStatus::STOPPED : TerminalStatus::DEADLINE, {}, {}, {}, {}};
  }

  std::expected<CellSize, TerminalError> size() const override {
    return size_;
  }

  std::expected<void, TerminalError> submit(std::string bytes) override {
    if (pending_) {
      return std::unexpected(TerminalError{TerminalErrorKind::OUTPUT_PENDING});
    }

    pending_ = bytes.size();

    return {};
  }

  TerminalWrite flush(TerminalDeadline, std::stop_token) override {
    const auto count = std::exchange(pending_, 0);

    return {TerminalStatus::PROGRESS, count, 0, {}};
  }

  std::expected<void, TerminalError> close() noexcept override {
    closed = true;
    pending_ = 0;

    return {};
  }

  bool closed{}; ///< Whether runner cleanup completed.

private:
  CellSize size_;
  std::size_t pending_{};
  std::mutex mutex_;
  std::condition_variable_any condition_;
};

/** @brief Count a real underlying transport without duplicate I/O or output. */
class CountedTerminal final : public ApplicationTerminal {
public:
  /** @param terminal Exclusive underlying transport, borrowed through run.
   * @param reads Optional bounded read samples, borrowed through run.
   * @param active Optional observation-window gate, borrowed through run. */
  explicit CountedTerminal(ApplicationTerminal &terminal, Samples *reads = nullptr, const bool *active = nullptr)
      : terminal_(terminal), reads_(reads), active_(active) {
  }

  TerminalRead read(TerminalDeadline deadline, std::stop_token stop) override {
    const auto begin = std::chrono::steady_clock::now();
    auto result = terminal_.read(deadline, stop);

    ++readCalls;

    if (reads_) {
      reads_->add(
        std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - begin).count(),
        active_ && *active_);
    }

    if (result.resized) {
      ++resizes;
      finalSize = *result.resized;
    }

    eof = eof || result.status == TerminalStatus::END_OF_INPUT;

    return result;
  }

  std::expected<CellSize, TerminalError> size() const override {
    return terminal_.size();
  }

  std::expected<void, TerminalError> submit(std::string bytes) override {
    const auto count = bytes.size();
    auto result = terminal_.submit(std::move(bytes));

    if (result) {
      counters.accept(count);
    }

    return result;
  }

  TerminalWrite flush(TerminalDeadline deadline, std::stop_token stop) override {
    auto result = terminal_.flush(deadline, stop);

    counters.flush(result);

    return result;
  }

  std::expected<void, TerminalError> close() noexcept override {
    auto result = terminal_.close();

    closed = result.has_value();

    return result;
  }

  OutputCounters counters;
  CellSize finalSize{120, 40};
  std::uint64_t resizes{}, readCalls{};
  bool closed{}, eof{};

private:
  ApplicationTerminal &terminal_;
  Samples *reads_{};
  const bool *active_{};
};

/** @brief Independent endpoint/extrema reduction of every deterministic raw tick.
 * @param actual Provider aggregate, captured after stop.
 * @param ticks Number actually generated, not the configured target.
 * @param seed Generator seed identifying the finite raw sequence.
 * @return True iff no raw contribution was lost from the OHLC reduction. */
inline bool verifyOhlc(Ohlc actual, std::uint64_t ticks, std::uint64_t seed) {
  if (!ticks) {
    return actual.open == 100 && actual.high == 100 && actual.low == 100 && actual.close == 100;
  }

  const auto first = dashboard::syntheticPrice(0, seed);
  double high = first, low = first, last = first;

  for (std::uint64_t i = 0; i < ticks; ++i) {
    last = dashboard::syntheticPrice(i, seed);
    high = std::max(high, last);
    low = std::min(low, last);
  }

  return actual.open == first && actual.close == last && actual.high == high && actual.low == low;
}
} // namespace benchmark
