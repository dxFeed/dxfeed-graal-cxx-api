// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "Generator.hpp"

#include <algorithm>
#include <chrono>
#include <exception>
#include <optional>

namespace dxfcpp::tools::nettest {

GeneratorResult runGenerator(const std::shared_ptr<DXPublisher> &publisher, EventPool &pool, std::size_t batchSize,
                             RateLimiter *limiter, StopControl &stop, InstanceCounters &counters,
                             PublishCounters &publish, std::chrono::nanoseconds holdAhead) {
    using Clock = std::chrono::steady_clock;

    GeneratorResult result;
    EventBatch batch;

    while (!stop.stopped()) {
        // (1) The wait for the batch of the schedule.
        std::optional<RateLimiter::Clock::time_point> due{};

        if (limiter != nullptr) {
            due = limiter->acquire();

            if (!due.has_value()) {
                break;
            }

            // The seam of the tests: the generator waits as if it were holdAhead ahead of its schedule.
            if (holdAhead.count() > 0) {
                if (stop.waitFor(holdAhead)) {
                    break;
                }

                due = Clock::now();
            }
        }

        // (2) One clock read: the steady clock for the slip and the elapsed time, the wall clock for the events.
        const auto start = Clock::now();
        const auto epochNanos =
            std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch())
                .count();

        if (due.has_value()) {
            const auto slip =
                std::max<std::int64_t>(0, std::chrono::duration_cast<std::chrono::nanoseconds>(start - *due).count());

            publish.slipLastNanos.store(slip, std::memory_order_relaxed);
            raiseTo(publish.slipIntervalMaxNanos, slip);
            raiseTo(publish.slipRunMaxNanos, slip);
        }

        std::int64_t unset = 0;

        publish.firstBatchEpochNanos.compare_exchange_strong(unset, epochNanos, std::memory_order_relaxed);

        try {
            // (3) The fill and (4) the publishing call.
            pool.fillAt(batch, batchSize, epochNanos);

            const auto filled = Clock::now();

            publisher->publishEvents(batch.begin(), batch.end());

            // (5) The elapsed time of (3) and (4).
            const auto end = Clock::now();
            const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
            const auto published = static_cast<std::int64_t>(batch.size());

            counters.records.fetch_add(published, std::memory_order_relaxed);
            counters.weightedNanos.fetch_add(published * elapsed, std::memory_order_relaxed);
            publish.fillNanos.fetch_add(std::chrono::duration_cast<std::chrono::nanoseconds>(filled - start).count(),
                                        std::memory_order_relaxed);
            publish.publishNanos.fetch_add(std::chrono::duration_cast<std::chrono::nanoseconds>(end - filled).count(),
                                           std::memory_order_relaxed);
            publish.batches.fetch_add(1, std::memory_order_relaxed);
        } catch (const std::exception &e) {
            result.ok = false;
            result.error = e.what();

            break;
        }
    }

    return result;
}

} // namespace dxfcpp::tools::nettest
