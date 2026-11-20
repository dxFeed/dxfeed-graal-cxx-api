// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// The run of NetTest: the endpoints, the publishing and the reports.

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

/// The outcome of a run of the producer.
struct RunResult {
    /// 0, or 1 on a failure.
    int exitCode = 0;
    /// The message of the failure, empty on success.
    std::string error{};
    /// The records and the batches published by all instances.
    std::int64_t records = 0;
    std::int64_t batches = 0;
    /// The records of each instance, in the order of the instances.
    std::vector<std::int64_t> instanceRecords{};
    /// The names of the endpoints that were created, in the order of the instances.
    std::vector<std::string> endpointNames{};
};

/// Runs the producer until the stop is requested, the duration of the settings passes or a failure: creates the
/// endpoints (a publisher with its generator thread each), publishes the batches on the schedule of -r, closes the
/// endpoints and joins the threads on every path. The system properties of the settings must already be set. The
/// statistics lines go to `out`.
RunResult runProducer(const Settings &settings, const Symbols &symbols, StopControl &stop, std::ostream &out);

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
