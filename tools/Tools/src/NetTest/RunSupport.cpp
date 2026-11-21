// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "RunSupport.hpp"

#include <algorithm>
#include <chrono>
#include <exception>

namespace dxfcpp::tools::nettest {

namespace {

DXEndpoint::Role apiRole(Role role) {
    switch (role) {
    case Role::FEED:
        return DXEndpoint::Role::FEED;
    case Role::STREAM_FEED:
        return DXEndpoint::Role::STREAM_FEED;
    case Role::PUBLISHER:
        return DXEndpoint::Role::PUBLISHER;
    case Role::STREAM_PUBLISHER:
        return DXEndpoint::Role::STREAM_PUBLISHER;
    }

    return DXEndpoint::Role::PUBLISHER;
}

// Whether every server socket connector of the endpoint accepts connections.
bool serverSocketsBound(const DXEndpoint &endpoint) {
    for (const auto &connector : endpoint.getConnectors()) {
        if (connector.kind == MessageConnectorInfo::Kind::SERVER_SOCKET &&
            connector.state != MessageConnectorInfo::State::CONNECTED) {
            return false;
        }
    }

    return true;
}

// The runtime values of the isolate; all absent when it does not give them, so that the runtime line shows `n/a`.
RuntimeSample readRuntime() {
    RuntimeSample sample;

    try {
        const auto statistics = System::getRuntimeStatistics();

        sample.heapUsedBytes = statistics.heapUsedBytes;
        sample.heapCommittedBytes = statistics.heapCommittedBytes;
        sample.youngGcCount = statistics.youngGcCount;
        sample.youngGcTimeMs = statistics.youngGcTimeMs;
        sample.fullGcCount = statistics.fullGcCount;
        sample.fullGcTimeMs = statistics.fullGcTimeMs;
        sample.processCpuTimeNs = statistics.processCpuTimeNs;
    } catch (const std::exception &) {
        return RuntimeSample{};
    }

    return sample;
}

// The statistics of a connector: those of the socket, or the I/O statistics with the subscription records, the lags
// and the round-trip times when the SDK has them, as the summary of the C tool takes them.
ConnectorSummary connectorSummary(const MessageConnectorInfo &connector) {
    ConnectorSummary summary;

    summary.address = connector.address;

    if (connector.ioStatistics.has_value()) {
        const auto &io = *connector.ioStatistics;

        summary.readBytes = io.readBytes;
        summary.writtenBytes = io.writtenBytes;
        summary.readDataRecords = io.readDataRecords;
        summary.writtenDataRecords = io.writtenDataRecords;
        summary.activeConnections = io.activeConnectionCount;
        summary.closedConnections = io.closedConnectionCount;
        summary.v2 = ConnectorSummaryV2{io.readSubRecords,       io.writtenSubRecords, io.readDataLagSumMicros,
                                        io.writtenDataLagSumMicros, io.readRttSumMicros,  io.writtenRttSumMicros};
    } else {
        const auto &socket = connector.statistics;

        summary.readBytes = socket.readBytes;
        summary.writtenBytes = socket.writtenBytes;
        summary.readDataRecords = socket.readDataRecords;
        summary.writtenDataRecords = socket.writtenDataRecords;
        summary.activeConnections = socket.activeConnectionCount;
        summary.closedConnections = socket.closedConnectionCount;
    }

    return summary;
}

// Reads the connectors of the endpoint once: adds their statistics to the totals, taken as the summary takes them
// (connectorSummary), and returns their open connections.
std::int64_t readConnectors(const DXEndpoint &endpoint, ConnectorTotals &totals) {
    std::int64_t connections = 0;

    for (const auto &connector : endpoint.getConnectors()) {
        const auto summary = connectorSummary(connector);

        connections += connector.connectionCount;
        totals.readBytes += summary.readBytes;
        totals.writtenBytes += summary.writtenBytes;
        totals.readRecords += summary.readDataRecords;
        totals.writtenRecords += summary.writtenDataRecords;

        if (summary.v2.has_value()) {
            const auto &v2 = *summary.v2;
            const auto add = [](std::int64_t &total, std::int64_t value) {
                total = (total == CONNECTOR_V2_ABSENT ? 0 : total) + value;
            };

            add(totals.subRecords, v2.readSubRecords + v2.writtenSubRecords);
            add(totals.readLagMicros, v2.readDataLagMicros);
            add(totals.rttMicros, v2.readRttMicros + v2.writtenRttMicros);
        }
    }

    return connections;
}

} // namespace

std::shared_ptr<DXEndpoint> buildEndpoint(const Settings &settings, const std::string &name) {
    auto builder = DXEndpoint::newBuilder()->withRole(apiRole(endpointRole(settings)))->withName(name);

    for (const auto &[key, value] : settings.properties) {
        builder->withProperty(key, value);
    }

    builder->withProperty(NANO_TIME_PROPERTY, endpointNanoTime(settings));

    return builder->build();
}

void closeEndpoint(const std::shared_ptr<DXEndpoint> &endpoint) noexcept {
    if (endpoint) {
        try {
            endpoint->closeAndAwaitTermination();
        } catch (...) {
            // The endpoint is gone anyway.
        }
    }
}

bool connectEndpoint(DXEndpoint &endpoint, const std::string &name, const std::string &address, std::string &error) {
    endpoint.connect(address);

    const auto deadline = std::chrono::steady_clock::now() + SERVER_START_TIMEOUT;

    while (!serverSocketsBound(endpoint)) {
        if (std::chrono::steady_clock::now() >= deadline) {
            error = "the server socket '" + address + "' (" + name + ") did not start in " +
                    std::to_string(SERVER_START_TIMEOUT.count()) + " ms (is the port already in use?)";

            return false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    return true;
}

std::int64_t systemNanos() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

std::int64_t steadyNanos() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

std::shared_ptr<LiveCounters> liveCountersOf(const Settings &settings, std::shared_ptr<LiveCounters> live) {
    if (live != nullptr && live->instances.size() == static_cast<std::size_t>(settings.instances)) {
        return live;
    }

    return std::make_shared<LiveCounters>(settings.instances);
}

void reportUntilStopped(const Settings &settings, LiveCounters &live,
                        const std::vector<std::shared_ptr<DXEndpoint>> &endpoints, StopControl &stop,
                        std::ostream &out, ReportCollector &collector) {
    const auto side = settings.mode == Mode::PRODUCER ? StatisticsSide::PRODUCER : StatisticsSide::CONSUMER;
    const auto warmupNanos = settings.warmupMillis.value_or(0) * 1'000'000;

    runReporter(
        settings.name, settings.periodMillis, systemReporterClock(stop),
        [&] {
            StatisticsSample sample;
            ConnectorTotals totals;

            for (std::size_t index = 0; index < endpoints.size(); ++index) {
                const auto &counters = live.instances[index];
                const auto connections = readConnectors(*endpoints[index], totals);

                sample.instances.push_back({counters.records.load(std::memory_order_relaxed),
                                            counters.weightedNanos.load(std::memory_order_relaxed)});
                sample.connections += connections;
                live.connections[index].store(connections, std::memory_order_relaxed);
            }

            live.setConnectors(totals);
            const auto runtime = readRuntime();
            const auto now = systemNanos();

            live.setRuntime(runtime);
            sample.runtime = runtime;

            // The runtime of the summary counts from the first sample, taken before the first tick.
            if (!collector.firstRuntime.has_value()) {
                collector.firstRuntime = runtime;
                collector.firstRuntimeNanos = now;
            }

            collector.lastRuntime = runtime;
            collector.lastRuntimeNanos = now;

            return sample;
        },
        [&](const std::string &text) {
            out << text << std::flush;
        },
        side,
        [&](const StatisticsSample &previous, const StatisticsSample &current, std::int64_t startNanos,
            std::int64_t endNanos) {
            IntervalRecord interval;

            interval.startEpochMs = startNanos / 1'000'000;
            interval.endEpochMs = endNanos / 1'000'000;
            const auto warmupStart = collector.warmupStart ? collector.warmupStart() : 0;
            // The first interval counts from the zero sample: it holds the records that came before the reporter
            // started (the consumer connects all its instances first), so its data starts at the first record.
            const auto dataStart =
                collector.intervals.empty() && warmupStart != 0 ? std::min(startNanos, warmupStart) : startNanos;

            // Without --warmup every interval is measured.
            interval.warmup =
                settings.warmupMillis.has_value() && isWarmupInterval(warmupNanos, warmupStart, dataStart);
            interval.connections = current.connections;

            for (const auto &delta : makeSnapshot(settings.name, previous, current, endNanos - startNanos, side).deltas) {
                interval.records += delta.records;
            }

            interval.rate = recordsPerSecond(interval.records, endNanos - startNanos);

            if (collector.complete) {
                collector.complete(interval);
            }

            collector.intervals.push_back(interval);
        });
}

RunReport makeReport(std::int64_t startEpochNs, ReportCollector &collector,
                     const std::vector<std::shared_ptr<DXEndpoint>> &endpoints) {
    RunReport report;

    report.startEpochNs = startEpochNs;
    report.endEpochNs = systemNanos();
    report.intervals = std::move(collector.intervals);

    for (const auto &interval : report.intervals) {
        report.records += interval.records;
    }

    for (const auto &endpoint : endpoints) {
        if (!endpoint) {
            continue;
        }

        // An endpoint whose connectors cannot be read is left out.
        try {
            for (const auto &connector : endpoint->getConnectors()) {
                report.connectors.push_back(connectorSummary(connector));
            }
        } catch (const std::exception &) {
            // The summary has no connectors of this endpoint then.
        }
    }

    if (collector.firstRuntime.has_value() && collector.lastRuntime.has_value()) {
        report.runtime = runtimeSummary(*collector.firstRuntime, collector.firstRuntimeNanos, *collector.lastRuntime,
                                        collector.lastRuntimeNanos);
    }

    return report;
}

} // namespace dxfcpp::tools::nettest
