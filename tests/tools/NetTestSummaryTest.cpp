// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// The histogram and the summary file of NetTest against the kit of the NetTest specification
// (docs/wrapper-guide/nettest.md): the percentiles of histogram-vectors.json and summaries that
// nettest-summary.schema.json accepts.

#include <doctest.h>

#include "../../tools/Tools/src/NetTest/Args.hpp"
#include "../../tools/Tools/src/NetTest/Histogram.hpp"
#include "../../tools/Tools/src/NetTest/Run.hpp"
#include "../../tools/Tools/src/NetTest/Statistics.hpp"
#include "../../tools/Tools/src/NetTest/Summary.hpp"
#include "nettest/KitJson.hpp"
#include "nettest/KitSchema.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <vector>

using namespace dxfcpp::tools::nettest;

namespace {

// The directory of the kit, or nullopt when the SDK has none and it is the stub of the SDK: the tests of the kit skip
// then. With the real SDK the missing kit fails the test.
std::optional<std::string> kitDirectory() {
    const std::string directory = DXFCXX_NETTEST_KIT_DIR;

    if (std::filesystem::is_directory(directory)) {
        return directory;
    }

#ifdef DXFCXX_NETTEST_SDK_STUB
    MESSAGE("The kit of the NetTest specification is not in the SDK stub (" << directory << "): the test is skipped");

    return std::nullopt;
#else
    FAIL("The kit of the NetTest specification is not found in the archive of the Graal Native SDK: " << directory);

    return std::nullopt;
#endif
}

std::vector<std::string> schemaErrors(const std::string &directory, const std::string &text) {
    const auto schema = kit::readJson(directory + "/nettest-summary.schema.json");
    const auto value = kit::JsonReader(text).read();
    std::vector<std::string> errors;

    kit::checkSchema(value, schema, "$", errors);

    return errors;
}

std::string joined(const std::vector<std::string> &lines) {
    std::string text;

    for (const auto &line : lines) {
        text += line + "\n";
    }

    return text;
}

Settings settingsOf(const std::vector<std::string> &args) {
    const auto parsed = parseArguments(args);

    REQUIRE_MESSAGE(parsed.ok, parsed.error);

    return parsed.settings;
}

RunReport producerReport() {
    RunReport report;

    report.startEpochNs = 1'763'380'800'000'000'000;
    report.endEpochNs = report.startEpochNs + 20'000'000'000;

    for (int index = 0; index < 2; ++index) {
        IntervalRecord interval;

        interval.startEpochMs = report.startEpochNs / 1'000'000 + index * 10'000;
        interval.endEpochMs = interval.startEpochMs + 10'000;
        interval.warmup = index == 0;
        interval.records = 50'000;
        interval.rate = 5'000;
        interval.publishNsPerEvent = 120;
        interval.slipMaxNs = 30'000;
        interval.slipEndNs = 2'000;
        interval.connections = 1;
        report.records += interval.records;
        report.intervals.push_back(interval);
    }

    report.publish = PublishSummary{1000, 100'000, 12'000'000};
    report.slip = SlipSummary{30'000, 2'000, std::nullopt};

    ConnectorSummary connector;

    connector.address = "*:7001";
    connector.writtenBytes = 3'000'000;
    connector.writtenDataRecords = 100'000;
    connector.activeConnections = 1;
    // Distinct values, so that a key on the wrong field shows.
    connector.v2 = ConnectorSummaryV2{1, 2, 3, 4, 5, 6};
    report.connectors.push_back(connector);

    // A connector of an SDK without the I/O statistics.
    ConnectorSummary socketOnly;

    socketOnly.address = "*:7002";
    report.connectors.push_back(socketOnly);
    report.runtime.heapUsedMb = 40;
    report.runtime.heapCommittedMb = 256;
    report.runtime.gcCount = 2;
    report.runtime.gcTimeMs = 15;
    report.runtime.cpuPercentMean = 37;

    return report;
}

RunReport consumerReport() {
    RunReport report;

    report.startEpochNs = 1'763'380'800'000'000'000;
    report.endEpochNs = report.startEpochNs + 10'000'000'000;

    IntervalRecord interval;

    interval.startEpochMs = report.startEpochNs / 1'000'000;
    interval.endEpochMs = interval.startEpochMs + 10'000;
    interval.records = 1000;
    interval.rate = 100;
    interval.lagMeanNs = -12;
    interval.lagP50Ns = 0;
    interval.lagP99Ns = 381'183;
    interval.lagMaxNs = 400'000;
    report.records = interval.records;
    report.intervals.push_back(interval);

    LagSummary lag;

    lag.count = 1000;
    lag.negative = 3;
    lag.p50Ns = 0;
    lag.p90Ns = 1023;
    lag.p99Ns = 381'183;
    lag.p999Ns = 381'183;
    lag.p9999Ns = 381'183;
    lag.maxNs = 400'000;
    lag.meanNs = -12;
    report.e2eLag = lag;
    report.symbolsSeen = 100;

    return report;
}

IntervalRecord slipInterval(std::int64_t endEpochMs, bool warmup, std::int64_t maxNs, std::int64_t endNs) {
    IntervalRecord interval;

    interval.startEpochMs = endEpochMs - 1000;
    interval.endEpochMs = endEpochMs;
    interval.warmup = warmup;
    interval.slipMaxNs = maxNs;
    interval.slipEndNs = endNs;

    return interval;
}

} // namespace

TEST_CASE("NetTest kit: the histogram gives the percentiles of histogram-vectors.json") {
    const auto directory = kitDirectory();

    if (!directory) {
        return;
    }

    const auto vectors = kit::readJson(*directory + "/histogram-vectors.json");
    const auto &histogramSettings = vectors["histogram"];

    CHECK(histogramSettings["lowest"].asInteger() == HISTOGRAM_LOWEST);
    CHECK(histogramSettings["highest"].asInteger() == HISTOGRAM_HIGHEST);
    CHECK(histogramSettings["digits"].asInteger() == HISTOGRAM_DIGITS);
    REQUIRE(!vectors["vectors"].array.empty());

    for (const auto &vector : vectors["vectors"].array) {
        Histogram histogram;

        if (vector.has("recorded_range")) {
            const auto &range = vector["recorded_range"];

            for (auto value = range["from"].asInteger(); value <= range["to"].asInteger(); ++value) {
                histogram.record(value, range["count_each"].asInteger());
            }
        } else {
            for (const auto &pair : vector["recorded"].array) {
                histogram.record(pair.array.at(0).asInteger(), pair.array.at(1).asInteger());
            }
        }

        INFO(vector["about"].asString());
        CHECK(histogram.totalCount() == vector["count"].asInteger());
        CHECK(histogram.valueAtPercentile(50.0) == vector["p50_ns"].asInteger());
        CHECK(histogram.valueAtPercentile(90.0) == vector["p90_ns"].asInteger());
        CHECK(histogram.valueAtPercentile(99.0) == vector["p99_ns"].asInteger());
        CHECK(histogram.valueAtPercentile(99.9) == vector["p999_ns"].asInteger());
        CHECK(histogram.valueAtPercentile(99.99) == vector["p9999_ns"].asInteger());
    }
}

TEST_CASE("NetTest: the histogram takes a negative value as 0 and a value above the highest as the highest") {
    Histogram histogram;

    CHECK(histogram.valueAtPercentile(50.0) == 0);

    histogram.record(-5);
    CHECK(histogram.valueAtPercentile(100.0) == 0);

    histogram.record(HISTOGRAM_HIGHEST * 2);
    CHECK(histogram.valueAtPercentile(100.0) == histogram.highestEquivalentValue(HISTOGRAM_HIGHEST));

    Histogram other;

    other.record(1000, 3);
    histogram.add(other);
    CHECK(histogram.totalCount() == 5);

    histogram.reset();
    CHECK(histogram.totalCount() == 0);
    CHECK(histogram.valueAtPercentile(50.0) == 0);
}

TEST_CASE("NetTest kit: the summary of a producer has every key that nettest-summary.schema.json requires") {
    const auto directory = kitDirectory();

    if (!directory) {
        return;
    }

    const auto settings =
        settingsOf({"p", ":7001", "-S", "100", "-r", "*/5k", "--reuse", "--warmup", "10s", "--summary", "summary.json"});
    SummaryStatus status;

    status.version = "v9.0.0";
    status.qdVersion = "3.356";
    status.buildVendor = "Oracle Corporation 21.0.2";
    status.buildMarch = "x86-64-v3";
    // The garbage collectors stay empty: a value the SDK does not give is null.

    const auto text = formatSummary(settings, producerReport(), status);
    const auto errors = schemaErrors(*directory, text);

    INFO(text);
    INFO(joined(errors));
    CHECK(errors.empty());

    const auto value = kit::JsonReader(text).read();

    CHECK(value["side"].asString() == "producer");
    CHECK(value["tool"]["implementation"].asString() == "cpp-api");
    CHECK(value["tool"]["version"].asString() == "v9.0.0");
    CHECK(value["tool"]["sdk_version"].isNull());
    CHECK(value["tool"]["qd_version"].asString() == "3.356");
    CHECK(value["tool"]["build"]["vendor"].asString() == "Oracle Corporation 21.0.2");
    CHECK(value["tool"]["build"]["march"].asString() == "x86-64-v3");
    CHECK(value["tool"]["build"]["gc"].isNull());
    CHECK(value["config"]["warmup_ms"].asInteger() == 10'000);
    CHECK(value["config"]["rate_limit"].asInteger() == 5'000);
    // The C++ API copies every published event: --reuse is never effective.
    CHECK(value["config"]["reuse"].boolean);
    CHECK_FALSE(value["config"]["reuse_effective"].boolean);
    CHECK(value["config"]["isolate_arguments"].array.empty());
    CHECK(value["e2e_lag"].isNull());
    CHECK(value["symbols_seen"].isNull());
    CHECK(value["publish_call"]["ns_per_event_mean"].asInteger() == 120);
    CHECK(value["schedule_slip"]["max_ns"].asInteger() == 30'000);
    CHECK(value["schedule_slip"]["growth_ns_per_s"].isNull());
    CHECK(value["intervals"].array.at(0)["phase"].asString() == "warmup");
    CHECK(value["intervals"].array.at(1)["phase"].asString() == "measure");

    std::int64_t records = 0;

    for (const auto &interval : value["intervals"].array) {
        records += interval["records"].asInteger();
    }

    CHECK(records == value["records"].asInteger());

    const auto &connectors = value["connectors"].array;

    REQUIRE(connectors.size() == 2);
    CHECK(connectors.at(0)["address"].asString() == "*:7001");
    CHECK(connectors.at(0)["v1"]["written_data_records"].asInteger() == 100'000);
    CHECK(connectors.at(0)["v2"]["read_sub_records"].asInteger() == 1);
    CHECK(connectors.at(0)["v2"]["written_sub_records"].asInteger() == 2);
    CHECK(connectors.at(0)["v2"]["data_read_lag_us"].asInteger() == 3);
    CHECK(connectors.at(0)["v2"]["data_write_lag_us"].asInteger() == 4);
    CHECK(connectors.at(0)["v2"]["read_rtt_us"].asInteger() == 5);
    CHECK(connectors.at(0)["v2"]["write_rtt_us"].asInteger() == 6);
    CHECK(connectors.at(1)["v1"].has("read_bytes"));
    CHECK(connectors.at(1)["v2"].isNull());

    // The check of the schema is not empty: an empty name breaks its minLength.
    auto unnamed = settings;

    unnamed.name.clear();

    const auto unnamedErrors = schemaErrors(*directory, formatSummary(unnamed, producerReport(), status));

    INFO(joined(unnamedErrors));
    CHECK(joined(unnamedErrors).find(".name: shorter than the minLength") != std::string::npos);
}

TEST_CASE("NetTest kit: the summary of a consumer stopped by a signal is valid against nettest-summary.schema.json") {
    const auto directory = kitDirectory();

    if (!directory) {
        return;
    }

    const auto settings = settingsOf({"c", "127.0.0.1:7001", "-S", "100", "-c", "stream", "-Xmx512m", "-XX:+PrintGC"});
    SummaryStatus status;

    status.status = "stopped_by_signal";

    const auto text = formatSummary(settings, consumerReport(), status);
    const auto errors = schemaErrors(*directory, text);

    INFO(text);
    INFO(joined(errors));
    CHECK(errors.empty());

    const auto value = kit::JsonReader(text).read();

    CHECK(value["status"].asString() == "stopped_by_signal");
    CHECK(value["tool"]["version"].isNull());
    CHECK(value["config"]["reuse_effective"].isNull());
    REQUIRE(value["config"]["isolate_arguments"].array.size() == 2);
    CHECK(value["config"]["isolate_arguments"].array[0].asString() == "-Xmx512m");
    CHECK(value["config"]["isolate_arguments"].array[1].asString() == "-XX:+PrintGC");
    CHECK(value["e2e_lag"]["negative"].asInteger() == 3);
    CHECK(value["e2e_lag"]["mean_ns"].asInteger() == -12);
    CHECK(value["publish_call"].isNull());
    CHECK(value["schedule_slip"].isNull());
    CHECK(value["symbols_seen"].asInteger() == 100);
    CHECK(value["connectors"].array.empty());
}

TEST_CASE("NetTest kit: the summary of a failed run carries the error and is valid against the schema") {
    const auto directory = kitDirectory();

    if (!directory) {
        return;
    }

    const auto settings = settingsOf({"p", ":7001", "-S", "100"});
    SummaryStatus status;

    status.status = "error";
    status.exitCode = 1;
    status.error = "cannot connect \"x\"\n\ttab";

    RunReport report;

    report.startEpochNs = 1'763'380'800'000'000'000;
    report.endEpochNs = report.startEpochNs;

    const auto text = formatSummary(settings, report, status);
    const auto errors = schemaErrors(*directory, text);

    INFO(text);
    INFO(joined(errors));
    CHECK(errors.empty());
    CHECK(kit::JsonReader(text).read()["error"].asString() == status.error);
}

TEST_CASE("NetTest: the summary file is written through <file>.tmp and an unwritable path is an error") {
    const auto directory = std::filesystem::path(DXFCXX_TEST_TEMP_DIR) / "nettest-summary-test";

    std::filesystem::create_directories(directory);

    const auto file = (directory / "summary.json").string();
    std::string error;

    REQUIRE(writeSummary(file, "{}\n", error));
    CHECK_FALSE(std::filesystem::exists(file + ".tmp"));

    {
        std::ifstream stream(file, std::ios::binary);
        const std::string content((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());

        CHECK(content == "{}\n");
    }

    // An existing file is replaced.
    REQUIRE(writeSummary(file, "{\"a\":1}\n", error));
    CHECK_FALSE(std::filesystem::exists(file + ".tmp"));

    {
        std::ifstream stream(file, std::ios::binary);
        const std::string content((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());

        CHECK(content == "{\"a\":1}\n");
    }

    CHECK_FALSE(writeSummary((directory / "no-such-dir" / "summary.json").string(), "{}\n", error));
    CHECK_FALSE(error.empty());

    std::filesystem::remove_all(directory);
}

TEST_CASE("NetTest: the JSON strings of the summary escape the quotes, the backslash and the control characters") {
    CHECK(jsonString("a\"b\\c") == "\"a\\\"b\\\\c\"");
    CHECK(jsonString("\n\t\x01") == "\"\\n\\t\\u0001\"");
}

TEST_CASE("NetTest: an interval is warm-up until the warm-up started and while it starts before its end") {
    constexpr std::int64_t warmup = 1'000'000'000;
    constexpr std::int64_t start = 5'000'000'000;

    // The warm-up has not started: no batch (`p`) or record (`c`) yet.
    CHECK(isWarmupInterval(warmup, 0, start));
    CHECK(isWarmupInterval(warmup, start, start - 1));
    CHECK(isWarmupInterval(warmup, start, start));
    CHECK(isWarmupInterval(warmup, start, start + warmup - 1));
    // The end of the warm-up belongs to the measurement.
    CHECK_FALSE(isWarmupInterval(warmup, start, start + warmup));
    CHECK_FALSE(isWarmupInterval(warmup, start, start + warmup + 1));
    // --warmup 0 measures from the start of the warm-up on.
    CHECK_FALSE(isWarmupInterval(0, start, start));
}

TEST_CASE("NetTest: the slip is the largest of the measured intervals and grows per second of their ends") {
    // A warm-up interval with a larger slip than any measured one, then three measured intervals over 2 s.
    const std::vector<IntervalRecord> intervals{
        slipInterval(1000, true, 90'000'000, 80'000'000),
        slipInterval(2000, false, 5'000'000, 1'000'000),
        slipInterval(3000, false, 7'000'000, 4'000'000),
        slipInterval(4000, false, 6'000'000, 9'000'000),
    };
    const auto slip = slipSummary(intervals, 9'500'000);

    CHECK(slip.maxNs == 7'000'000);
    CHECK(slip.endNs == 9'500'000);
    // (9 ms - 1 ms) over the 2 s between the ends of the first and the last measured interval.
    REQUIRE(slip.growthNsPerSecond.has_value());
    CHECK(*slip.growthNsPerSecond == 4'000'000);

    // A slip that shrinks grows negatively.
    const auto shrinking = slipSummary({slipInterval(1000, false, 8, 8), slipInterval(1500, false, 8, 2)}, 2);

    REQUIRE(shrinking.growthNsPerSecond.has_value());
    CHECK(*shrinking.growthNsPerSecond == -12);
}

TEST_CASE("NetTest: without measured intervals the slip is the largest of all, and one measured has no growth") {
    const auto warmupOnly =
        slipSummary({slipInterval(1000, true, 3'000, 1'000), slipInterval(2000, true, 8'000, 2'000)}, 2'000);

    CHECK(warmupOnly.maxNs == 8'000);
    CHECK_FALSE(warmupOnly.growthNsPerSecond.has_value());

    const auto oneMeasured =
        slipSummary({slipInterval(1000, true, 3'000, 1'000), slipInterval(2000, false, 2'000, 2'000)}, 2'000);

    CHECK(oneMeasured.maxNs == 2'000);
    CHECK_FALSE(oneMeasured.growthNsPerSecond.has_value());

    CHECK(slipSummary({}, 0).maxNs == 0);
}

TEST_CASE("NetTest: the runtime of the summary is the heap at the end and the collections and the CPU time between "
          "the samples") {
    constexpr std::int64_t MB = 1024 * 1024;
    RuntimeSample first;
    RuntimeSample last;

    first.heapUsedBytes = 10 * MB;
    first.heapCommittedBytes = 20 * MB;
    first.youngGcCount = 3;
    first.youngGcTimeMs = 30;
    first.fullGcCount = 1;
    first.fullGcTimeMs = 100;
    first.processCpuTimeNs = 1'000'000'000;

    last.heapUsedBytes = 50 * MB + MB / 2;
    last.heapCommittedBytes = 64 * MB;
    last.youngGcCount = 10;
    last.youngGcTimeMs = 75;
    last.fullGcCount = 2;
    last.fullGcTimeMs = 160;
    last.processCpuTimeNs = 4'000'000'000;

    // 3 s of CPU time over 2 s of the run: 150 %.
    const auto runtime = runtimeSummary(first, 1'000'000'000, last, 3'000'000'000);

    CHECK(runtime.heapUsedMb == 50);
    CHECK(runtime.heapCommittedMb == 64);
    CHECK(runtime.gcCount == 8);
    CHECK(runtime.gcTimeMs == 105);
    CHECK(runtime.cpuPercentMean == 150);

    // An absent value, a decreasing count and an empty period leave the values absent.
    last.heapUsedBytes = RUNTIME_VALUE_ABSENT;
    last.fullGcCount = 0;
    first.youngGcTimeMs = RUNTIME_VALUE_ABSENT;

    const auto partial = runtimeSummary(first, 1'000'000'000, last, 1'000'000'000);

    CHECK_FALSE(partial.heapUsedMb.has_value());
    CHECK(partial.heapCommittedMb == 64);
    CHECK_FALSE(partial.gcCount.has_value());
    CHECK_FALSE(partial.gcTimeMs.has_value());
    CHECK_FALSE(partial.cpuPercentMean.has_value());
}

TEST_CASE("NetTest: the lag totals keep the largest lag, a negative one too") {
    LagTotals lags;

    lags.add(-7);
    CHECK(lags.max == -7);

    lags.add(-3);
    CHECK(lags.max == -3);

    LagTotals other;

    other.add(12);
    lags.add(other);
    lags.add(LagTotals{});
    CHECK(lags.count == 3);
    CHECK(lags.negative == 2);
    CHECK(lags.sum == 2);
    CHECK(lags.max == 12);
}
