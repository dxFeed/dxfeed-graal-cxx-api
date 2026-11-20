// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// The summary file of a run (the section "Summary file" of the NetTest specification, the schema nettest-summary/1):
// what the run collects for it and the hand-written JSON.

#pragma once

#include "Args.hpp"
#include "Statistics.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace dxfcpp::tools::nettest {

/// One statistics interval; the absent values are null in the file.
struct IntervalRecord {
    std::int64_t startEpochMs = 0;
    std::int64_t endEpochMs = 0;
    /// The phase `warmup` (true) or `measure`.
    bool warmup = false;
    std::int64_t records = 0;
    std::int64_t rate = 0;
    /// `c`: the e2e lag of the records of the interval.
    std::optional<std::int64_t> lagMeanNs{};
    std::optional<std::int64_t> lagP50Ns{};
    std::optional<std::int64_t> lagP99Ns{};
    std::optional<std::int64_t> lagMaxNs{};
    /// `p`: the fill and the publishing call per event; the slip with -r.
    std::optional<std::int64_t> publishNsPerEvent{};
    std::optional<std::int64_t> slipMaxNs{};
    std::optional<std::int64_t> slipEndNs{};
    std::int64_t connections = 0;
};

/// The e2e lag of the records after the warm-up (`c`).
struct LagSummary {
    std::int64_t count = 0;
    std::int64_t negative = 0;
    /// The percentiles of the histogram; absent without records.
    std::optional<std::int64_t> p50Ns{};
    std::optional<std::int64_t> p90Ns{};
    std::optional<std::int64_t> p99Ns{};
    std::optional<std::int64_t> p999Ns{};
    std::optional<std::int64_t> p9999Ns{};
    /// Exact, with the negative lags at their signs; absent without records.
    std::optional<std::int64_t> maxNs{};
    std::optional<std::int64_t> meanNs{};
};

/// The publishing calls of all instances (`p`): the batches, their records and the nanoseconds of their fills and calls.
struct PublishSummary {
    std::int64_t calls = 0;
    std::int64_t events = 0;
    std::int64_t nanos = 0;
};

/// The schedule slip (`p` with -r).
struct SlipSummary {
    std::int64_t maxNs = 0;
    std::int64_t endNs = 0;
    /// The growth of the slip at the end of the measured intervals per second, absent with fewer than two of them.
    std::optional<std::int64_t> growthNsPerSecond{};
};

/// The I/O statistics of a connector beyond the socket statistics: the subscription records and the sums of the lags
/// and of the round-trip times, in microseconds.
struct ConnectorSummaryV2 {
    std::int64_t readSubRecords = 0;
    std::int64_t writtenSubRecords = 0;
    std::int64_t readDataLagMicros = 0;
    std::int64_t writtenDataLagMicros = 0;
    std::int64_t readRttMicros = 0;
    std::int64_t writtenRttMicros = 0;
};

/// The statistics of a connector at the end of the run, and its I/O statistics when the SDK has them.
struct ConnectorSummary {
    std::string address{};
    std::int64_t readBytes = 0;
    std::int64_t writtenBytes = 0;
    std::int64_t readDataRecords = 0;
    std::int64_t writtenDataRecords = 0;
    std::int64_t activeConnections = 0;
    std::int64_t closedConnections = 0;
    std::optional<ConnectorSummaryV2> v2{};
};

/// The runtime of the run: the heap at the end, the collections and the CPU time between the first and the last sample.
struct RuntimeSummary {
    std::optional<std::int64_t> heapUsedMb{};
    std::optional<std::int64_t> heapCommittedMb{};
    std::optional<std::int64_t> gcCount{};
    std::optional<std::int64_t> gcTimeMs{};
    std::optional<std::int64_t> cpuPercentMean{};
};

/// What a run collects for the summary.
struct RunReport {
    std::int64_t startEpochNs = 0;
    std::int64_t endEpochNs = 0;
    /// The records of all intervals.
    std::int64_t records = 0;
    std::vector<IntervalRecord> intervals{};
    std::optional<LagSummary> e2eLag{};
    std::optional<PublishSummary> publish{};
    std::optional<SlipSummary> slip{};
    std::optional<std::int64_t> symbolsSeen{};
    RuntimeSummary runtime{};
    std::vector<ConnectorSummary> connectors{};
};

/// How the run ended, as the summary writes it.
struct SummaryStatus {
    /// `ok`, `stopped_by_signal`, `error`, `no_data` or `nano_missing`.
    std::string status = "ok";
    int exitCode = 0;
    /// Empty is null.
    std::string error{};
    /// The version of the tool (the C++ API); empty is null.
    std::string version{};
    /// The version of the Graal Native SDK; empty is null.
    std::string sdkVersion{};
};

/// The JSON text of the summary file, with a line break at the end.
std::string formatSummary(const Settings &settings, const RunReport &report, const SummaryStatus &status);

/// Writes the text to `<file>.tmp` and renames it to `<file>`; returns false and fills the error on a failure.
bool writeSummary(const std::string &file, const std::string &text, std::string &error);

/// The JSON string literal of a text: quoted, with `"`, `\` and the control characters escaped.
std::string jsonString(const std::string &text);

/// Whether an interval starting at the wall-clock nanoseconds belongs to the warm-up of the nanoseconds that starts at
/// the first batch (`p`) or the first record (`c`): it does unless it starts at the end of the warm-up or later, and it
/// does while the warm-up has not started (0).
bool isWarmupInterval(std::int64_t warmupNanos, std::int64_t warmupStartNanos, std::int64_t startNanos);

/// The schedule slip of a run from its intervals and the slip of its last batch: the largest slip of the measured
/// intervals (of all of them when none is measured) and the growth of the slip at the end of the measured intervals.
SlipSummary slipSummary(const std::vector<IntervalRecord> &intervals, std::int64_t lastSlipNanos);

/// The runtime summary from the first and the last runtime samples and their wall-clock nanoseconds; a value that a
/// sample lacks (negative) or that decreases is absent.
RuntimeSummary runtimeSummary(const RuntimeSample &first, std::int64_t firstNanos, const RuntimeSample &last,
                              std::int64_t lastNanos);

} // namespace dxfcpp::tools::nettest
