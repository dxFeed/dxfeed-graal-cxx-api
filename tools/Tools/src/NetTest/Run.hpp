// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// The run of NetTest: the endpoints, the publishing on `p`, the receiving and the e2e lag on `c`, and the reports.

#pragma once

#include "Args.hpp"
#include "StopControl.hpp"
#include "SymbolPool.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <iosfwd>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace dxfcpp::tools::nettest {

/// How long the start waits for a server socket to bind before it fails (a port in use).
inline constexpr std::chrono::milliseconds SERVER_START_TIMEOUT{3000};

/// The starvation watchdog of the consumer: with open connections, no records for this long after the last record...
inline constexpr std::chrono::milliseconds NO_DATA_AFTER_RECORD{30'000};
/// ...or for this long after the connection when no record came at all, stops the run with RunStatus::STARVED.
inline constexpr std::chrono::milliseconds NO_DATA_AFTER_CONNECT{60'000};

/// The consumer stops with RunStatus::NANO_MISSING when the first this many events all have the nanosecond part 0.
inline constexpr std::int64_t NANO_CHECK_EVENTS = 1'000;

/// How a run ended. The names avoid the macros ERROR (wingdi.h) and NO_DATA (winsock.h).
enum class RunStatus {
    /// The duration passed or the stop was requested.
    OK,
    /// An endpoint failed to start or a publishing call failed.
    FAILED,
    /// The starvation watchdog of the consumer.
    STARVED,
    /// The first NANO_CHECK_EVENTS events of the consumer had no nanosecond part.
    NANO_MISSING,
};

/// The name of the status: `ok`, `error`, `no_data`, `nano_missing`.
std::string runStatusName(RunStatus status);

/// The e2e lags of received records: their count, their sum in nanoseconds with the signs, and how many are negative.
struct LagTotals {
    std::int64_t count = 0;
    std::int64_t sum = 0;
    std::int64_t negative = 0;

    /// Adds one lag; a negative lag (a clock step or a skew between the hosts) enters the sum with its sign.
    void add(std::int64_t lag) {
        ++count;
        sum += lag;
        negative += lag < 0 ? 1 : 0;
    }

    void add(const LagTotals &other) {
        count += other.count;
        sum += other.sum;
        negative += other.negative;
    }
};

/// The outcome of a run.
struct RunResult {
    /// 0, or 1 on a failure, the starvation watchdog or the check of the nanosecond part.
    int exitCode = 0;
    RunStatus status = RunStatus::OK;
    /// The message of the failure, empty on success; the statuses of the consumer's checks lead it (`no_data: ...`).
    std::string error{};
    /// The records published (on `p`) or received (on `c`) by all instances, and the batches published.
    std::int64_t records = 0;
    std::int64_t batches = 0;
    /// The records of each instance, in the order of the instances.
    std::vector<std::int64_t> instanceRecords{};
    /// The names of the endpoints that were created, in the order of the instances.
    std::vector<std::string> endpointNames{};
    /// On `c`: the e2e lags of all received records, and the distinct symbols received by all instances.
    LagTotals lags{};
    std::int64_t symbolsSeen = 0;
};

/// The seams of the tests; the defaults are the behavior of the tool.
struct RunHooks {
    /// The timeouts of the starvation watchdog of the consumer.
    std::chrono::milliseconds noDataAfterRecord = NO_DATA_AFTER_RECORD;
    std::chrono::milliseconds noDataAfterConnect = NO_DATA_AFTER_CONNECT;
};

/// Runs the producer until the stop is requested, the duration of the settings passes or a failure: creates the
/// endpoints (a publisher with its generator thread each), publishes the batches on the schedule of -r, closes the
/// endpoints and joins the threads on every path. The system properties of the settings must already be set. The
/// statistics lines go to `out`.
RunResult runProducer(const Settings &settings, const Symbols &symbols, StopControl &stop, std::ostream &out);

/// Runs the consumer until the stop is requested, the duration of the settings passes, the starvation watchdog fires or
/// the check of the nanosecond part fails: creates the feed endpoints (the listener first, then the symbols of the slice
/// of the instance, then the connection), measures the e2e lag of every record, closes the endpoints and joins the
/// threads on every path. The system properties of the settings must already be set. The statistics lines go to `out`.
RunResult runConsumer(const Settings &settings, const Symbols &symbols, StopControl &stop, std::ostream &out,
                      const RunHooks &hooks = {});

/// The symbols of consumer instance `instance` (from 1) of `instances`: those with the index j (from 0) where
/// j % instances == instance - 1, in the order of the set.
std::vector<std::string> consumerSlice(const std::vector<std::string> &symbols, std::int32_t instance,
                                       std::int32_t instances);

/// The e2e lag of an event in nanoseconds: `nowNanos - (time_ms * 1000000 + nanoPart)`, where time_ms is the time packed
/// in Trade::getTimeSequence() or TimeAndSale::getIndex() (packTimeSequence()).
std::int64_t eventLagNanos(std::int64_t nowNanos, std::int64_t packedTime, std::int32_t nanoPart);

/// Sets the system properties of the settings (-D) in the command line order. Returns false and fills `error` on a
/// failure.
bool applySystemProperties(const Settings &settings, std::string &error);

/// Requests the stop of the run on SIGINT, SIGTERM and, on Windows, SIGBREAK. The handler only sets a flag; a watcher
/// thread calls StopControl::request(). One object at a time; the previous handlers come back in the destructor.
class SignalWatcher {
    StopControl &stop_;
    std::atomic<bool> done_{false};
    std::vector<std::pair<int, void (*)(int)>> previous_{};
    std::thread thread_{};

    public:
    explicit SignalWatcher(StopControl &stop);
    SignalWatcher(const SignalWatcher &) = delete;
    SignalWatcher &operator=(const SignalWatcher &) = delete;
    ~SignalWatcher();

    /// Whether a signal was received.
    bool received() const;
};

/// Runs the tool with the validated settings: the statistics lines go to `out`, the messages of a failure to `err`.
/// Returns the exit code of the process.
int run(const Settings &settings, std::ostream &out, std::ostream &err);

} // namespace dxfcpp::tools::nettest
