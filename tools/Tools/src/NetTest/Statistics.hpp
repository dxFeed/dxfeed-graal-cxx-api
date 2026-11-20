// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// The statistics of NetTest: the formatters of the statistics blocks and of the runtime line, and the reporter that
// prints them on the ticks aligned to the wall clock.

#pragma once

#include "StopControl.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace dxfcpp::tools::nettest {

/// A runtime value that the isolate does not give, as in RuntimeStatistics.
inline constexpr std::int64_t RUNTIME_VALUE_ABSENT = -1;

/// The side of the statistics: the label of the summary line and the meaning of its lag.
enum class StatisticsSide {
    /// `p`: NetTestProducerSide, the lag is the time to build and publish a batch, weighted by its records.
    PRODUCER,
    /// `c`: NetTestConsumerSide, the lag is the mean e2e lag of the records, which may be negative.
    CONSUMER,
};

/// The cumulative counters of one instance.
struct InstanceTotals {
    std::int64_t records = 0;
    /// On `p`, the sum of records * (the time to build and publish the batch of those records) in nanoseconds; on `c`,
    /// the sum of the e2e lags of the records in nanoseconds, with their signs.
    std::int64_t weightedNanos = 0;
};

/// The runtime values of the isolate that the runtime line prints: the heap now and the cumulative GC and CPU
/// counters. A negative value is absent (RUNTIME_VALUE_ABSENT).
struct RuntimeSample {
    std::int64_t heapUsedBytes = RUNTIME_VALUE_ABSENT;
    std::int64_t heapCommittedBytes = RUNTIME_VALUE_ABSENT;
    std::int64_t youngGcCount = RUNTIME_VALUE_ABSENT;
    std::int64_t youngGcTimeMs = RUNTIME_VALUE_ABSENT;
    std::int64_t fullGcCount = RUNTIME_VALUE_ABSENT;
    std::int64_t fullGcTimeMs = RUNTIME_VALUE_ABSENT;
    std::int64_t processCpuTimeNs = RUNTIME_VALUE_ABSENT;
};

/// The cumulative counters of all instances and the connections open now, as the reporter reads them, and the runtime
/// values of the isolate; without them the reporter prints no runtime line.
struct StatisticsSample {
    std::vector<InstanceTotals> instances{};
    std::int64_t connections = 0;
    std::optional<RuntimeSample> runtime{};
};

/// What a statistics block prints: the deltas of the instances over a period of the real length and the connections.
struct StatisticsSnapshot {
    StatisticsSide side = StatisticsSide::PRODUCER;
    std::string name = "NetTest";
    std::vector<InstanceTotals> deltas{};
    std::int64_t periodNanos = 0;
    std::int64_t connections = 0;
};

/// The number with `,` grouping of the thousands, as NumberFormat.getIntegerInstance(Locale.US) prints it.
std::string groupDigits(std::int64_t value);

/// Records per second over the period, integer division; 0 for an empty period.
std::int64_t recordsPerSecond(std::int64_t records, std::int64_t periodNanos);

/// The record-weighted mean time to build and publish a batch in microseconds, integer division; 0 without records.
std::int64_t lagMicros(std::int64_t records, std::int64_t weightedNanos);

/// The mean e2e lag in microseconds, `lagSumNanos / records / 1000` divided toward zero, so it may be negative; 0
/// without records.
std::int64_t meanLagMicros(std::int64_t records, std::int64_t lagSumNanos);

/// The text of a statistics block without the line break at the end: the summary line, then one line per instance.
std::string formatStatistics(const StatisticsSnapshot &snapshot);

/// The snapshot of the period between two samples of the same instances.
StatisticsSnapshot makeSnapshot(const std::string &name, const StatisticsSample &previous,
                                const StatisticsSample &current, std::int64_t periodNanos,
                                StatisticsSide side = StatisticsSide::PRODUCER);

/// The runtime line of the period between two samples, without the line break: the heap of the current sample in MB
/// (MiB, rounded down), the young plus full collections of the period with their time, and the process CPU time of the
/// period as a share of its length (above 100% on several cores). A value absent in either sample, or a cumulative
/// counter that went back, is `n/a`.
std::string formatRuntime(const std::string &name, const RuntimeSample &previous, const RuntimeSample &current,
                          std::int64_t periodNanos);

/// The clock of the reporter, replaceable in tests.
struct ReporterClock {
    /// The wall clock in nanoseconds since the Unix epoch.
    std::function<std::int64_t()> nowNanos;
    /// Sleeps until the wall-clock time; returns false when the reporter must stop.
    std::function<bool(std::int64_t)> sleepUntilNanos;
};

/// The clock of the system: the system clock, a sleep that ends at the stop of the control.
ReporterClock systemReporterClock(StopControl &stop);

/// Reads the cumulative counters and the connections now.
using StatisticsSource = std::function<StatisticsSample()>;

/// Receives the text of a block (with the line break at the end).
using StatisticsSink = std::function<void(const std::string &)>;

/// Receives every interval of the reporter, printed or not, so that the intervals cover the whole run: the sample at its
/// start (with no instances for the first interval, whose counters start at 0), the sample at its end, and its bounds on
/// the wall clock in nanoseconds since the Unix epoch.
using IntervalSink = std::function<void(const StatisticsSample &previous, const StatisticsSample &current,
                                        std::int64_t startNanos, std::int64_t endNanos)>;

/// Runs on the calling thread until the clock stops it. Sleeps to the next wall-clock multiple of the period and prints
/// the deltas against the previous tick; the first tick after the start only primes the counters and prints nothing. At
/// the stop it prints the partial period since the last tick (or since the start when no tick came) at its real length,
/// unless that period is shorter than one second. When both samples have runtime values, the runtime line follows the
/// block in the same text. The side selects the label and the lag of the blocks. The interval sink, when given, gets the
/// interval from the start to the first tick, each interval between two ticks and the one from the last tick to the stop.
void runReporter(const std::string &name, std::int64_t periodMillis, const ReporterClock &clock,
                 const StatisticsSource &source, const StatisticsSink &sink,
                 StatisticsSide side = StatisticsSide::PRODUCER, const IntervalSink &intervals = {});

} // namespace dxfcpp::tools::nettest
