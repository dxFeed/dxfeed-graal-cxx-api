// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// The generator loop of one NetTest instance: wait for the batch of the schedule, build the batch, publish it, count it.

#pragma once

#include "../../../../include/dxfeed_graal_cpp_api/api.hpp"
#include "EventPool.hpp"
#include "RateLimiter.hpp"
#include "StopControl.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace dxfcpp::tools::nettest {

/// The cumulative counters of an instance: written by its generator, read by the reporter. The nanoseconds are weighted
/// by the records: each batch adds records * (the steady-clock time from the start of its fill to the return of the
/// publishing call).
struct InstanceCounters {
    std::atomic<std::int64_t> records{0};
    std::atomic<std::int64_t> weightedNanos{0};
};

/// The counters of the batches of all generators of a run.
struct PublishCounters {
    /// The published batches.
    std::atomic<std::int64_t> batches{0};
    /// The nanoseconds of the fills and of the publishing calls of all batches.
    std::atomic<std::int64_t> fillNanos{0};
    std::atomic<std::int64_t> publishNanos{0};
    /// With a schedule: the slip of the last batch and the largest slip since the reporter last took it (it resets the
    /// value by exchanging it with 0).
    std::atomic<std::int64_t> slipLastNanos{0};
    std::atomic<std::int64_t> slipIntervalMaxNanos{0};
    /// The largest slip of the run.
    std::atomic<std::int64_t> slipRunMaxNanos{0};
    /// The wall-clock time of the clock read of the first batch, 0 before it: the start of the warm-up on `p`.
    std::atomic<std::int64_t> firstBatchEpochNanos{0};

    /// The nanoseconds of the fills and the calls together: the lag of `p`.
    std::int64_t fillAndPublishNanos() const {
        return fillNanos.load(std::memory_order_relaxed) + publishNanos.load(std::memory_order_relaxed);
    }
};

/// The live counters of a run: one set per instance and the common one, which the generators write and the reporter
/// reads.
struct LiveCounters {
    std::vector<InstanceCounters> instances;
    PublishCounters publish{};

    explicit LiveCounters(std::int32_t instanceCount) : instances(static_cast<std::size_t>(instanceCount)) {
    }

    /// The records of all instances.
    std::int64_t records() const {
        std::int64_t total = 0;

        for (const auto &instance : instances) {
            total += instance.records.load(std::memory_order_relaxed);
        }

        return total;
    }
};

/// Raises the atomic to the value when the value is larger.
inline void raiseTo(std::atomic<std::int64_t> &target, std::int64_t value) {
    auto current = target.load(std::memory_order_relaxed);

    while (value > current && !target.compare_exchange_weak(current, value, std::memory_order_relaxed)) {
    }
}

struct GeneratorResult {
    bool ok = true;
    /// The text of the exception of the failed publishing call.
    std::string error{};
};

/// Runs the loop on the calling thread until the stop is requested or a publishing call fails. Each batch: (1) the wait
/// for its time of the schedule, (2) one read of the clock, whose time stamps the events of the batch and gives the
/// slip, (3) the fill, (4) the publishing call, (5) the elapsed time of (3) and (4). The limiter is shared by all
/// generators; null means flat out, without a schedule.
/// @param batchSize the number of the events of a batch.
GeneratorResult runGenerator(const std::shared_ptr<DXPublisher> &publisher, EventPool &pool, std::size_t batchSize,
                             RateLimiter *limiter, const StopControl &stop, InstanceCounters &counters,
                             PublishCounters &publish);

} // namespace dxfcpp::tools::nettest
