// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// The schedule that all generators of a run share: the total rate in records per second, granted in whole batches.

#pragma once

#include "StopControl.hpp"

#include <chrono>
#include <cstdint>
#include <mutex>
#include <optional>

namespace dxfcpp::tools::nettest {

/// The intended schedule of the specification: from the first grant `t0`, batch `k`, counted over all generators, is
/// due at `T(k) = t0 + k * batch / rate`. A generator that is ahead sleeps until `T(k)` (the platform's sleep, no busy
/// wait); one that is behind gets its batch at once and catches up without a bound, never dropping a batch.
/// Thread-safe.
class RateLimiter {
    public:
    using Clock = std::chrono::steady_clock;

    private:
    StopControl &stop_;
    // The length of one batch of the schedule in nanoseconds: batch * 1e9 / rate.
    const double batchNanos_;
    std::mutex mutex_{};
    std::optional<Clock::time_point> start_{};
    std::int64_t next_ = 0;

    public:
    RateLimiter(std::int64_t recordsPerSecond, std::int64_t batch, StopControl &stop);

    RateLimiter(const RateLimiter &) = delete;
    RateLimiter &operator=(const RateLimiter &) = delete;

    /// The due time of batch k after the first grant at `start`.
    Clock::time_point due(Clock::time_point start, std::int64_t k) const;

    /// Takes the next batch of the schedule and waits until it is due; returns its due time `T(k)`, or nullopt, after
    /// taking it, when the stop is requested before that. The first call sets `t0` and returns at once.
    std::optional<Clock::time_point> acquire();
};

} // namespace dxfcpp::tools::nettest
