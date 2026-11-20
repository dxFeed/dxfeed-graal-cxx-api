// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "RateLimiter.hpp"

#include <cmath>

namespace dxfcpp::tools::nettest {

RateLimiter::RateLimiter(std::int64_t recordsPerSecond, std::int64_t batch, StopControl &stop)
    : stop_(stop), batchNanos_(static_cast<double>(batch) * 1e9 / static_cast<double>(recordsPerSecond)) {
}

RateLimiter::Clock::time_point RateLimiter::due(Clock::time_point start, std::int64_t k) const {
    return start + std::chrono::duration_cast<Clock::duration>(
                       std::chrono::nanoseconds(std::llround(static_cast<double>(k) * batchNanos_)));
}

std::optional<RateLimiter::Clock::time_point> RateLimiter::acquire() {
    Clock::time_point dueTime;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!start_.has_value()) {
            start_ = Clock::now();
        }

        dueTime = due(*start_, next_++);
    }

    // A wait may end early (a spurious wake-up of the condition variable): the time left is measured again.
    while (!stop_.stopped()) {
        const auto left = dueTime - Clock::now();

        if (left <= Clock::duration::zero()) {
            return dueTime;
        }

        if (stop_.waitFor(left)) {
            break;
        }
    }

    return std::nullopt;
}

} // namespace dxfcpp::tools::nettest
