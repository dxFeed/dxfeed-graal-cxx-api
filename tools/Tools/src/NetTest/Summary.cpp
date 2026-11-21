// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "Summary.hpp"

#include "Histogram.hpp"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <system_error>

namespace dxfcpp::tools::nettest {

namespace {

std::string number(const std::optional<std::int64_t> &value) {
    return value.has_value() ? std::to_string(*value) : "null";
}

std::string boolean(bool value) {
    return value ? "true" : "false";
}

std::string stringOrNull(const std::string &text) {
    return text.empty() ? "null" : jsonString(text);
}

std::string stringArray(const std::vector<std::string> &values) {
    std::string text = "[";

    for (std::size_t index = 0; index < values.size(); ++index) {
        text += (index == 0 ? "" : ", ") + jsonString(values[index]);
    }

    return text + "]";
}

// `"key": value` pairs joined into an object on one line.
class Object {
    std::string text_ = "{";

    public:
    Object &add(const std::string &key, const std::string &value) {
        text_ += (text_.size() == 1 ? "\"" : ", \"") + key + "\": " + value;

        return *this;
    }

    std::string text() const {
        return text_ + "}";
    }
};

std::string configOf(const Settings &settings) {
    const bool producer = settings.mode == Mode::PRODUCER;

    return Object()
        .add("address", jsonString(settings.address))
        .add("symbols", std::to_string(settings.totalSymbols))
        .add("symbols_source", jsonString(settings.symbolsFile.empty() ? "generated" : "file"))
        .add("instances", std::to_string(settings.instances))
        .add("rate_limit", producer ? number(settings.recordsPerSecond) : "null")
        .add("batch", producer ? std::to_string(settings.batch) : "null")
        .add("event", jsonString(eventName(settings.event)))
        .add("collector", settings.collector.has_value() ? jsonString(collectorName(*settings.collector)) : "null")
        .add("role", jsonString(roleName(endpointRole(settings))))
        .add("reuse", producer ? boolean(settings.reuse) : "null")
        // The C++ API copies every published event into the isolate, so the objects are never reused there.
        .add("reuse_effective", producer ? boolean(false) : "null")
        .add("conflated", boolean(isConflated(settings)))
        .add("period_ms", std::to_string(settings.periodMillis))
        .add("warmup_ms", number(settings.warmupMillis))
        .add("duration_ms", number(settings.durationMillis))
        .add("nano_time", boolean(isNanoTimeOn(settings)))
        .add("isolate_arguments", stringArray(settings.isolateArguments))
        .text();
}

std::string intervalOf(const IntervalRecord &interval) {
    return Object()
        .add("start_epoch_ms", std::to_string(interval.startEpochMs))
        .add("end_epoch_ms", std::to_string(interval.endEpochMs))
        .add("phase", jsonString(interval.warmup ? "warmup" : "measure"))
        .add("records", std::to_string(interval.records))
        .add("rate", std::to_string(interval.rate))
        .add("lag_mean_ns", number(interval.lagMeanNs))
        .add("lag_p50_ns", number(interval.lagP50Ns))
        .add("lag_p99_ns", number(interval.lagP99Ns))
        .add("lag_max_ns", number(interval.lagMaxNs))
        .add("publish_ns_per_event", number(interval.publishNsPerEvent))
        .add("slip_max_ns", number(interval.slipMaxNs))
        .add("slip_end_ns", number(interval.slipEndNs))
        .add("connections", std::to_string(interval.connections))
        .text();
}

std::string lagOf(const std::optional<LagSummary> &lag) {
    if (!lag.has_value()) {
        return "null";
    }

    const auto histogram = Object()
                               .add("lowest", std::to_string(HISTOGRAM_LOWEST))
                               .add("highest", std::to_string(HISTOGRAM_HIGHEST))
                               .add("digits", std::to_string(HISTOGRAM_DIGITS))
                               .text();

    return Object()
        .add("count", std::to_string(lag->count))
        .add("negative", std::to_string(lag->negative))
        .add("p50_ns", number(lag->p50Ns))
        .add("p90_ns", number(lag->p90Ns))
        .add("p99_ns", number(lag->p99Ns))
        .add("p999_ns", number(lag->p999Ns))
        .add("p9999_ns", number(lag->p9999Ns))
        .add("max_ns", number(lag->maxNs))
        .add("mean_ns", number(lag->meanNs))
        .add("histogram", histogram)
        .text();
}

std::string publishOf(const std::optional<PublishSummary> &publish) {
    if (!publish.has_value()) {
        return "null";
    }

    return Object()
        .add("calls", std::to_string(publish->calls))
        .add("events", std::to_string(publish->events))
        .add("ns_total", std::to_string(publish->nanos))
        .add("ns_per_event_mean",
             publish->events > 0 ? std::to_string(publish->nanos / publish->events) : std::string("null"))
        .text();
}

std::string slipOf(const std::optional<SlipSummary> &slip) {
    if (!slip.has_value()) {
        return "null";
    }

    return Object()
        .add("max_ns", std::to_string(slip->maxNs))
        .add("end_ns", std::to_string(slip->endNs))
        .add("growth_ns_per_s", number(slip->growthNsPerSecond))
        .text();
}

std::string runtimeOf(const RuntimeSummary &runtime) {
    return Object()
        .add("heap_used_mb", number(runtime.heapUsedMb))
        .add("heap_committed_mb", number(runtime.heapCommittedMb))
        .add("gc_count", number(runtime.gcCount))
        .add("gc_time_ms", number(runtime.gcTimeMs))
        .add("cpu_percent_mean", number(runtime.cpuPercentMean))
        .text();
}

std::string connectorOf(const ConnectorSummary &connector) {
    const auto v1 = Object()
                        .add("read_bytes", std::to_string(connector.readBytes))
                        .add("written_bytes", std::to_string(connector.writtenBytes))
                        .add("read_data_records", std::to_string(connector.readDataRecords))
                        .add("written_data_records", std::to_string(connector.writtenDataRecords))
                        .add("active_connections", std::to_string(connector.activeConnections))
                        .add("closed_connections", std::to_string(connector.closedConnections))
                        .text();

    std::string v2 = "null";

    if (connector.v2.has_value()) {
        const auto &values = *connector.v2;

        v2 = Object()
                 .add("read_sub_records", std::to_string(values.readSubRecords))
                 .add("written_sub_records", std::to_string(values.writtenSubRecords))
                 .add("data_read_lag_us", std::to_string(values.readDataLagMicros))
                 .add("data_write_lag_us", std::to_string(values.writtenDataLagMicros))
                 .add("read_rtt_us", std::to_string(values.readRttMicros))
                 .add("write_rtt_us", std::to_string(values.writtenRttMicros))
                 .text();
    }

    return Object().add("address", jsonString(connector.address)).add("v1", v1).add("v2", v2).text();
}

template <typename Item, typename Format>
std::string arrayOf(const std::vector<Item> &items, const Format &format, const std::string &indent) {
    if (items.empty()) {
        return "[]";
    }

    std::string text = "[";

    for (std::size_t index = 0; index < items.size(); ++index) {
        text += (index == 0 ? "\n" : ",\n") + indent + format(items[index]);
    }

    return text + "\n  ]";
}

} // namespace

std::string jsonString(const std::string &text) {
    std::string quoted = "\"";

    for (const char symbol : text) {
        const auto code = static_cast<unsigned char>(symbol);

        switch (symbol) {
        case '"':
            quoted += "\\\"";
            break;
        case '\\':
            quoted += "\\\\";
            break;
        case '\n':
            quoted += "\\n";
            break;
        case '\r':
            quoted += "\\r";
            break;
        case '\t':
            quoted += "\\t";
            break;
        default:
            if (code < 0x20) {
                char escaped[8];

                std::snprintf(escaped, sizeof(escaped), "\\u%04x", code);
                quoted += escaped;
            } else {
                quoted += symbol;
            }
        }
    }

    return quoted + "\"";
}

std::string formatSummary(const Settings &settings, const RunReport &report, const SummaryStatus &status) {
    const bool producer = settings.mode == Mode::PRODUCER;
    const auto build = Object().add("vendor", "null").add("march", "null").add("gc", "null").text();
    const auto tool = Object()
                          .add("implementation", jsonString(IMPLEMENTATION))
                          .add("name", "\"NetTest\"")
                          .add("version", stringOrNull(status.version))
                          .add("sdk_version", stringOrNull(status.sdkVersion))
                          .add("qd_version", "null")
                          .add("build", build)
                          .text();
    const auto length = report.endEpochNs - report.startEpochNs;
    std::string text = "{\n";
    const auto line = [&text](const std::string &key, const std::string &value, bool last = false) {
        text += "  \"" + key + "\": " + value + (last ? "\n" : ",\n");
    };

    line("schema", "\"nettest-summary/1\"");
    line("spec_version", jsonString(SPEC_VERSION));
    line("tool", tool);
    line("side", producer ? "\"producer\"" : "\"consumer\"");
    line("name", jsonString(settings.name));
    line("args", stringArray(settings.arguments));
    line("config", configOf(settings));
    line("start_epoch_ns", std::to_string(report.startEpochNs));
    line("end_epoch_ns", std::to_string(report.endEpochNs));
    line("status", jsonString(status.status));
    line("exit_code", std::to_string(status.exitCode));
    line("error", stringOrNull(status.error));
    line("records", std::to_string(report.records));
    line("rate", std::to_string(recordsPerSecond(report.records, length)));
    line("time_source", "\"actual\"");
    line("time_resolution", isNanoTimeOn(settings) ? "\"ns\"" : "\"ms\"");
    line("intervals", arrayOf(report.intervals, intervalOf, "    "));
    line("e2e_lag", lagOf(report.e2eLag));
    line("publish_call", publishOf(report.publish));
    line("schedule_slip", slipOf(report.slip));
    line("symbols_seen", number(report.symbolsSeen));
    line("runtime", runtimeOf(report.runtime));
    line("connectors", arrayOf(report.connectors, connectorOf, "    "), true);

    return text + "}\n";
}

bool writeSummary(const std::string &file, const std::string &text, std::string &error) {
    namespace fs = std::filesystem;

    const fs::path target(file);
    const auto temporary = fs::path(target).concat(".tmp");
    std::error_code code;

    {
        std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);

        stream << text;
        stream.close();

        if (!stream) {
            fs::remove(temporary, code);
            error = "cannot write the summary file '" + file + ".tmp'";

            return false;
        }
    }

    fs::rename(temporary, target, code);

    if (code) {
        error = "cannot rename the summary file to '" + file + "': " + code.message();
        fs::remove(temporary, code);

        return false;
    }

    return true;
}

bool isWarmupInterval(std::int64_t warmupNanos, std::int64_t warmupStartNanos, std::int64_t startNanos) {
    return warmupStartNanos == 0 || startNanos < warmupStartNanos + warmupNanos;
}

SlipSummary slipSummary(const std::vector<IntervalRecord> &intervals, std::int64_t lastSlipNanos) {
    SlipSummary slip;
    std::vector<const IntervalRecord *> measured;

    for (const auto &interval : intervals) {
        if (!interval.warmup) {
            measured.push_back(&interval);
        }
    }

    // The largest slip of the measured intervals, of all intervals when none is measured.
    for (const auto &interval : intervals) {
        if ((measured.empty() || !interval.warmup) && interval.slipMaxNs.has_value()) {
            slip.maxNs = std::max(slip.maxNs, *interval.slipMaxNs);
        }
    }

    slip.endNs = lastSlipNanos;

    if (measured.size() >= 2) {
        const auto &first = *measured.front();
        const auto &last = *measured.back();
        const auto millis = last.endEpochMs - first.endEpochMs;

        if (millis > 0 && first.slipEndNs.has_value() && last.slipEndNs.has_value()) {
            slip.growthNsPerSecond = (*last.slipEndNs - *first.slipEndNs) * 1000 / millis;
        }
    }

    return slip;
}

RuntimeSummary runtimeSummary(const RuntimeSample &first, std::int64_t firstNanos, const RuntimeSample &last,
                              std::int64_t lastNanos) {
    static constexpr std::int64_t BYTES_PER_MB = 1024 * 1024;
    using Optional = std::optional<std::int64_t>;
    const auto megabytes = [](std::int64_t bytes) {
        return bytes >= 0 ? Optional(bytes / BYTES_PER_MB) : std::nullopt;
    };
    const auto growth = [](std::int64_t from, std::int64_t to) {
        return from >= 0 && to >= from ? Optional(to - from) : std::nullopt;
    };
    const auto both = [](Optional one, Optional other) {
        return one.has_value() && other.has_value() ? Optional(*one + *other) : std::nullopt;
    };
    const auto cpu = growth(first.processCpuTimeNs, last.processCpuTimeNs);
    const auto period = lastNanos - firstNanos;
    RuntimeSummary runtime;

    runtime.heapUsedMb = megabytes(last.heapUsedBytes);
    runtime.heapCommittedMb = megabytes(last.heapCommittedBytes);
    runtime.gcCount =
        both(growth(first.youngGcCount, last.youngGcCount), growth(first.fullGcCount, last.fullGcCount));
    runtime.gcTimeMs =
        both(growth(first.youngGcTimeMs, last.youngGcTimeMs), growth(first.fullGcTimeMs, last.fullGcTimeMs));

    if (cpu.has_value() && period > 0) {
        runtime.cpuPercentMean = *cpu * 100 / period;
    }

    return runtime;
}

} // namespace dxfcpp::tools::nettest
