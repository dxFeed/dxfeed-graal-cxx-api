// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// What the producer and the consumer of NetTest share: the endpoints, the clocks, the threads and the reporter that
// collects the intervals of the summary.

#pragma once

#include "../../../../include/dxfeed_graal_cpp_api/api.hpp"
#include "Generator.hpp"
#include "Metrics.hpp"
#include "Run.hpp"
#include "Statistics.hpp"
#include "Summary.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <thread>
#include <vector>

namespace dxfcpp::tools::nettest {

/// Builds an endpoint of the settings: the role, the name, the -D properties and the value of dxscheme.nanoTime on its
/// builder.
std::shared_ptr<DXEndpoint> buildEndpoint(const Settings &settings, const std::string &name);

/// Closes the endpoint and waits until it has processed its data; does nothing without one.
void closeEndpoint(const std::shared_ptr<DXEndpoint> &endpoint) noexcept;

/// Connects the endpoint. A server socket must bind: the connector of a bound server socket reports itself as connected
/// within milliseconds, one that cannot bind (the port is taken) stays connecting, so the start waits until every server
/// socket connector is connected, at most SERVER_START_TIMEOUT. A client address never waits: with no peer the connector
/// stays connecting, which is a normal state. Returns false and fills `error` on a timeout.
bool connectEndpoint(DXEndpoint &endpoint, const std::string &name, const std::string &address, std::string &error);

/// The wall clock in nanoseconds since the Unix epoch.
std::int64_t systemNanos();

/// The steady clock in nanoseconds.
std::int64_t steadyNanos();

/// Stops the waiting of `toRequest` and joins the threads: when the scope ends on any path, no thread outlives it.
struct JoinGuard {
    StopControl &toRequest;
    std::vector<std::thread> threads{};

    explicit JoinGuard(StopControl &control) : toRequest(control) {
    }

    JoinGuard(const JoinGuard &) = delete;
    JoinGuard &operator=(const JoinGuard &) = delete;

    void join() {
        toRequest.request();

        for (auto &thread : threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }

        threads.clear();
    }

    ~JoinGuard() {
        join();
    }
};

/// The live counters of a run: `live` when it has a set per instance of the settings, else new ones.
std::shared_ptr<LiveCounters> liveCountersOf(const Settings &settings, std::shared_ptr<LiveCounters> live);

/// What the reporter collects for the summary besides the lines it prints: the intervals and the first and the last
/// runtime samples. Only the reporter thread writes it; it is read after that thread has been joined.
struct ReportCollector {
    /// The wall-clock time of the start of the warm-up (the first batch on `p`, the first record on `c`), 0 before it.
    std::function<std::int64_t()> warmupStart{};
    /// Completes an interval with the values of the side: the publishing on `p`, the lags on `c`.
    std::function<void(IntervalRecord &)> complete{};
    std::vector<IntervalRecord> intervals{};
    std::optional<RuntimeSample> firstRuntime{};
    std::optional<RuntimeSample> lastRuntime{};
    std::int64_t firstRuntimeNanos = 0;
    std::int64_t lastRuntimeNanos = 0;
};

/// The reporter thread: prints the statistics block and the runtime line of every period from the live counters, and
/// the last block at the stop, and collects the intervals of the run; it lives until `stop` is requested, which comes
/// after the generators have ended (on `p`) or after the run has stopped (on `c`). It is the only thread of the run that
/// reads the connectors and the runtime of the SDK, once at the start, once per tick and once at the stop, and keeps the
/// open connections of each instance, the connector totals and the runtime values in the live counters, where the
/// watchdog and /metrics read them.
void reportUntilStopped(const Settings &settings, LiveCounters &live,
                        const std::vector<std::shared_ptr<DXEndpoint>> &endpoints, StopControl &stop,
                        std::ostream &out, ReportCollector &collector);

/// The part of the report that both sides share, for a run that started at `startEpochNs`: the end now, the intervals
/// and their records, the runtime, and the statistics of the connectors of the endpoints, which must still be open.
RunReport makeReport(std::int64_t startEpochNs, ReportCollector &collector,
                     const std::vector<std::shared_ptr<DXEndpoint>> &endpoints);

} // namespace dxfcpp::tools::nettest
