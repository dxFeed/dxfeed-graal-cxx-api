// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// The tool against the kit of the NetTest specification (docs/wrapper-guide/nettest.md) from the archive of the Graal
// Native SDK: the parser gives the settings and the errors of cli-vectors.json.

#include <doctest.h>

#include <dxfeed_graal_cpp_api/api.hpp>

#include "../../tools/Tools/src/Args/Args.hpp"
#include "../../tools/Tools/src/NetTest/Args.hpp"
#include "../../tools/Tools/src/NetTest/EventPool.hpp"
#include "../../tools/Tools/src/NetTest/NetTestTool.hpp"
#include "../../tools/Tools/src/NetTest/Statistics.hpp"
#include "../../tools/Tools/src/NetTest/SymbolPool.hpp"
#include "nettest/KitJson.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <regex>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

using namespace dxfcpp;
using namespace dxfcpp::tools;
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

// The array of a kit value, which must not be empty: an emptied kit file fails instead of passing with no checks.
const std::vector<kit::Json> &nonEmpty(const kit::Json &value) {
    REQUIRE(!value.array.empty());

    return value.array;
}

// A setting of the summary config as the parser gives it, in the text of a JSON value (null when absent).
std::string settingOf(const Settings &settings, const std::string &key) {
    const auto quoted = [](const std::string &text) {
        return "\"" + text + "\"";
    };
    const auto number = [](const std::optional<std::int64_t> &value) {
        return value.has_value() ? std::to_string(*value) : std::string("null");
    };
    const bool producer = settings.mode == Mode::PRODUCER;

    if (key == "side") {
        return quoted(producer ? "producer" : "consumer");
    }

    if (key == "address") {
        return quoted(settings.address);
    }

    if (key == "symbols") {
        return std::to_string(settings.totalSymbols);
    }

    if (key == "symbols_source") {
        return quoted(settings.symbolsFile.empty() ? "generated" : "file");
    }

    if (key == "instances") {
        return std::to_string(settings.instances);
    }

    if (key == "rate_limit") {
        return producer ? number(settings.recordsPerSecond) : "null";
    }

    if (key == "batch") {
        return producer ? std::to_string(settings.batch) : "null";
    }

    if (key == "event") {
        return quoted(eventName(settings.event));
    }

    if (key == "collector") {
        return settings.collector.has_value() ? quoted(collectorName(*settings.collector)) : "null";
    }

    if (key == "role") {
        return quoted(roleName(endpointRole(settings)));
    }

    if (key == "reuse") {
        return producer ? (settings.reuse ? "true" : "false") : "null";
    }

    if (key == "period_ms") {
        return std::to_string(settings.periodMillis);
    }

    if (key == "warmup_ms") {
        return number(settings.warmupMillis);
    }

    if (key == "duration_ms") {
        return number(settings.durationMillis);
    }

    if (key == "name") {
        return quoted(settings.name);
    }

    if (key == "conflated") {
        return isConflated(settings) ? "true" : "false";
    }

    return "unknown setting " + key;
}

// The kit value as settingOf() prints it.
std::string kitValueText(const kit::Json &value) {
    switch (value.kind) {
    case kit::Json::Kind::NUL:
        return "null";
    case kit::Json::Kind::BOOLEAN:
        return value.boolean ? "true" : "false";
    case kit::Json::Kind::INTEGER:
        return std::to_string(value.integer);
    case kit::Json::Kind::STRING:
        return "\"" + value.string + "\"";
    default:
        return "unsupported kit value";
    }
}

// The tool has no isolate arguments and no deprecated publishing call: an entry that needs one of them is skipped,
// an entry that needs their absence is checked.
bool needsFeature(const kit::Json &entry) {
    if (!entry.has("requires")) {
        return false;
    }

    const auto &features = entry["requires"].array;

    return std::any_of(features.begin(), features.end(), [](const kit::Json &feature) {
        return !feature.asString().empty() && feature.asString()[0] != '!';
    });
}

struct ProcessResult {
    int exitCode{};
    std::string out{};
    std::string err{};
};

ProcessResult runTool(const std::vector<std::string> &args) {
    std::ostringstream out;
    std::ostringstream err;
    const auto exitCode = NetTestTool::runProcess(args, out, err);

    return {exitCode, out.str(), err.str()};
}

std::string firstLine(const std::string &text) {
    return text.substr(0, text.find('\n'));
}

// A 64-bit value of the kit: a hex string.
std::uint64_t hexValue(const kit::Json &value) {
    return std::stoull(value.asString(), nullptr, 16);
}

// The lines of a text without the line ends (the kit files come with CRLF on Windows).
std::vector<std::string> splitLines(const std::string &text) {
    std::vector<std::string> lines;
    std::istringstream in(text);
    std::string line;

    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (!line.empty()) {
            lines.push_back(line);
        }
    }

    return lines;
}

// The IPF reader needs the Graal isolate, which the SDK stub does not have: the tests that read a file skip then.
bool ipfReadable() {
#ifdef DXFCXX_NETTEST_SDK_STUB
    MESSAGE("The IPF reader needs the real SDK: the test is skipped");

    return false;
#else
    return true;
#endif
}

const std::string IPF_FILE = (std::filesystem::path(DXFCXX_TEST_DATA_DIR) / "ipf" / "option-chains.ipf").string();

// The value of a kit field of a runtime sample: null is a value the runtime does not give.
std::int64_t sampleValue(const kit::Json &sample, const std::string &key) {
    return sample[key].isNull() ? RUNTIME_VALUE_ABSENT : sample[key].asInteger();
}

// The runtime sample of lines.json: the kit gives one GC counter pair, which is the young one here, the full one is 0.
RuntimeSample runtimeSampleOf(const kit::Json &sample) {
    RuntimeSample result;

    result.heapUsedBytes = sampleValue(sample, "heap_used_bytes");
    result.heapCommittedBytes = sampleValue(sample, "heap_committed_bytes");
    result.youngGcCount = sampleValue(sample, "gc_count");
    result.youngGcTimeMs = sampleValue(sample, "gc_time_ms");
    result.fullGcCount = 0;
    result.fullGcTimeMs = 0;
    result.processCpuTimeNs = sampleValue(sample, "cpu_ns");

    return result;
}

// The clock of a reporter that jumps to the time it sleeps until and stops after the given number of ticks.
struct FakeClock {
    std::int64_t now = 0;
    int ticks = 0;
    std::int64_t stopAt = 0;

    ReporterClock clock() {
        ReporterClock result;

        result.nowNanos = [this] {
            return now;
        };
        result.sleepUntilNanos = [this](std::int64_t nanos) {
            if (ticks == 0) {
                now = stopAt;

                return false;
            }

            --ticks;
            now = nanos;

            return true;
        };

        return result;
    }
};

// The blocks that a reporter prints with a fake clock that starts at `start`, ticks `ticks` times and stops at `stop`;
// every sample of the source holds 100 records more than the previous one.
std::vector<std::string> reportedBlocks(std::int64_t periodMillis, std::int64_t start, int ticks, std::int64_t stop) {
    FakeClock fake;
    std::vector<std::string> blocks;
    std::int64_t records = 0;

    fake.now = start;
    fake.ticks = ticks;
    fake.stopAt = stop;
    runReporter(
        "t", periodMillis, fake.clock(),
        [&] {
            StatisticsSample sample;

            records += 100;
            sample.instances.push_back({records, 0});

            return sample;
        },
        [&](const std::string &text) {
            blocks.push_back(text);
        });

    return blocks;
}

} // namespace

TEST_CASE("NetTest kit: grouping of lines.json") {
    const auto directory = kitDirectory();

    if (!directory) {
        return;
    }

    const auto lines = kit::readJson(*directory + "/lines.json");

    for (const auto &entry : nonEmpty(lines["grouping"])) {
        INFO("value " << entry["value"].asInteger());
        CHECK(groupDigits(entry["value"].asInteger()) == entry["text"].asString());
    }
}

TEST_CASE("NetTest kit: the statistics blocks of lines.json are the same byte for byte") {
    const auto directory = kitDirectory();

    if (!directory) {
        return;
    }

    const auto lines = kit::readJson(*directory + "/lines.json");

    for (const auto &entry : nonEmpty(lines["statistics"])) {
        const bool producer = entry["side"].asString() == "producer";
        StatisticsSnapshot snapshot;

        snapshot.side = producer ? StatisticsSide::PRODUCER : StatisticsSide::CONSUMER;
        snapshot.name = entry["name"].asString();
        snapshot.periodNanos = entry["interval_ns"].asInteger();
        snapshot.connections = entry["connections"].asInteger();

        for (const auto &instance : entry["instances"].array) {
            snapshot.deltas.push_back(
                {instance["records"].asInteger(), instance[producer ? "weighted_ns" : "lag_sum_ns"].asInteger()});
        }

        INFO(entry["text"].asString());
        CHECK(formatStatistics(snapshot) == entry["text"].asString());
    }
}

TEST_CASE("NetTest kit: the runtime lines of lines.json are the same byte for byte") {
    const auto directory = kitDirectory();

    if (!directory) {
        return;
    }

    const auto lines = kit::readJson(*directory + "/lines.json");

    for (const auto &entry : nonEmpty(lines["runtime"])) {
        INFO(entry["text"].asString());
        CHECK(formatRuntime(entry["name"].asString(), runtimeSampleOf(entry["previous"]),
                            runtimeSampleOf(entry["current"]),
                            entry["interval_ns"].asInteger()) == entry["text"].asString());
    }
}

TEST_CASE("NetTest kit: every captured line passes an expression of regexes.json") {
    const auto directory = kitDirectory();

    if (!directory) {
        return;
    }

    const auto lines = kit::readJson(*directory + "/lines.json");
    const auto expressions = kit::readJson(*directory + "/regexes.json");
    std::vector<std::regex> regexes;

    for (const auto &name : {"summary", "instance", "runtime", "isolate_rejection"}) {
        regexes.emplace_back(expressions["regexes"][name].asString(), std::regex::ECMAScript);
    }

    for (const auto &line : nonEmpty(lines["captured"])) {
        INFO(line.asString());
        CHECK(std::any_of(regexes.begin(), regexes.end(), [&](const std::regex &regex) {
            return std::regex_search(line.asString(), regex);
        }));
    }

    // The lines of the formatters pass the expressions of their own kind.
    for (const auto &entry : nonEmpty(lines["statistics"])) {
        std::istringstream text(entry["text"].asString());
        std::string line;

        std::getline(text, line);
        CHECK(std::regex_search(line, regexes[0]));

        while (std::getline(text, line)) {
            INFO(line);
            CHECK(std::regex_search(line, regexes[1]));
        }
    }

    for (const auto &entry : nonEmpty(lines["runtime"])) {
        INFO(entry["text"].asString());
        CHECK(std::regex_search(entry["text"].asString(), regexes[2]));
    }
}

TEST_CASE("NetTest: a missing value or a counter that went back is n/a in the runtime line") {
    RuntimeSample previous;

    previous.heapUsedBytes = 1;
    previous.heapCommittedBytes = 2;
    previous.youngGcCount = previous.fullGcCount = 0;
    previous.youngGcTimeMs = previous.fullGcTimeMs = 0;
    previous.processCpuTimeNs = 100;

    auto current = previous;

    current.heapUsedBytes = 3 * 1024 * 1024;
    current.heapCommittedBytes = 4 * 1024 * 1024;
    current.youngGcCount = 2;
    current.youngGcTimeMs = 7;
    current.fullGcCount = 1;
    current.fullGcTimeMs = 5;
    current.processCpuTimeNs = 100 + 500'000'000;
    CHECK(formatRuntime("n", previous, current, 1'000'000'000) ==
          "{*NetTestRuntime-n*} heap 3 MB of 4 MB, gc 3 (12 ms), cpu 50%");

    auto missing = current;

    missing.heapUsedBytes = RUNTIME_VALUE_ABSENT;
    missing.heapCommittedBytes = RUNTIME_VALUE_ABSENT;
    CHECK(formatRuntime("n", previous, missing, 1'000'000'000) ==
          "{*NetTestRuntime-n*} heap n/a of n/a, gc 3 (12 ms), cpu 50%");

    auto noGc = current;

    noGc.fullGcCount = RUNTIME_VALUE_ABSENT;
    CHECK(formatRuntime("n", previous, noGc, 1'000'000'000) ==
          "{*NetTestRuntime-n*} heap 3 MB of 4 MB, gc n/a, cpu 50%");

    auto noCpu = current;

    noCpu.processCpuTimeNs = RUNTIME_VALUE_ABSENT;
    CHECK(formatRuntime("n", previous, noCpu, 1'000'000'000) ==
          "{*NetTestRuntime-n*} heap 3 MB of 4 MB, gc 3 (12 ms), cpu n/a");

    auto cpuBack = current;

    cpuBack.processCpuTimeNs = 50;
    CHECK(formatRuntime("n", previous, cpuBack, 1'000'000'000) ==
          "{*NetTestRuntime-n*} heap 3 MB of 4 MB, gc 3 (12 ms), cpu n/a");

    auto gcBack = current;

    previous.youngGcCount = 5;
    gcBack.youngGcCount = 0;
    CHECK(formatRuntime("n", previous, gcBack, 1'000'000'000) ==
          "{*NetTestRuntime-n*} heap 3 MB of 4 MB, gc n/a, cpu 50%");
}

TEST_CASE("NetTest: the ticks are at the multiples of the period, the first one only starts the count") {
    // The start at 10.2 s: the tick at 11 s primes, those at 12 s and 13 s print, the stop at 13 s adds no block.
    const auto blocks = reportedBlocks(1000, 10'200'000'000, 3, 13'000'000'000);

    REQUIRE(blocks.size() == 2);
    // The source adds 100 records per sample, so each second has 100 records.
    CHECK(blocks[0] == "{*NetTestProducerSide-t*} data 100 rps, lag 0 us, instances 1, connections 0\n"
                       "    t.1 data 100 rps, lag 0 us\n");
    CHECK(blocks[1] == blocks[0]);
}

TEST_CASE("NetTest: the last block is printed only when it is not shorter than a second") {
    // The last tick at 12 s: a stop before 13 s gives no block, at 13 s and later one block of the real length.
    CHECK(reportedBlocks(1000, 10'200'000'000, 2, 12'999'999'999).size() == 1);

    const auto blocks = reportedBlocks(1000, 10'200'000'000, 2, 13'000'000'000);

    REQUIRE(blocks.size() == 2);
    CHECK(blocks[1] == "{*NetTestProducerSide-t*} data 100 rps, lag 0 us, instances 1, connections 0\n"
                       "    t.1 data 100 rps, lag 0 us\n");

    const auto longer = reportedBlocks(1000, 10'200'000'000, 2, 14'000'000'000);

    REQUIRE(longer.size() == 2);
    CHECK(longer[1] == "{*NetTestProducerSide-t*} data 50 rps, lag 0 us, instances 1, connections 0\n"
                       "    t.1 data 50 rps, lag 0 us\n");

    // Without a tick the last block covers the time since the start.
    const auto onlyLast = reportedBlocks(1000, 10'200'000'000, 0, 11'200'000'000);

    REQUIRE(onlyLast.size() == 1);
    CHECK(onlyLast[0] == "{*NetTestProducerSide-t*} data 100 rps, lag 0 us, instances 1, connections 0\n"
                         "    t.1 data 100 rps, lag 0 us\n");
}

TEST_CASE("NetTest: a period longer than the longest wait ticks at the multiples of the longest wait") {
    // The wall clock of 2026 in nanoseconds; the parser lets `-s` reach std::int64_t milliseconds.
    constexpr std::int64_t now = 1'790'000'000'000'000'000;
    constexpr std::int64_t longestNanos = MAX_WAIT_MILLIS * 1'000'000;
    std::vector<std::int64_t> sleeps;
    ReporterClock clock;

    clock.nowNanos = [] {
        return now;
    };
    clock.sleepUntilNanos = [&sleeps](std::int64_t nanos) {
        sleeps.push_back(nanos);

        return false;
    };

    runReporter(
        "t", std::numeric_limits<std::int64_t>::max(), clock,
        [] {
            return StatisticsSample{};
        },
        [](const std::string &) {
        });

    REQUIRE(sleeps.size() == 1);
    CHECK(sleeps[0] == (now / longestNanos + 1) * longestNanos);
    CHECK(sleeps[0] > now);
}

TEST_CASE("NetTest kit: the parser gives the settings and the errors of cli-vectors.json") {
    const auto directory = kitDirectory();

    if (!directory) {
        return;
    }

    const auto vectors = kit::readJson(*directory + "/cli-vectors.json");
    const std::string prefix = "NetTest: ";
    std::size_t checked = 0;
    std::vector<std::string> skipped;

    for (const auto &entry : nonEmpty(vectors["vectors"])) {
        std::vector<std::string> args;
        std::string commandLine;

        for (const auto &argument : entry["args"].array) {
            args.push_back(argument.asString());
            commandLine += " " + argument.asString();
        }

        if (needsFeature(entry)) {
            skipped.push_back("NetTest" + commandLine);

            continue;
        }

        INFO("NetTest" << commandLine);

        if (entry["exit"].asInteger() == 0) {
            const auto parsed = parseArguments(args);

            INFO(parsed.error);
            REQUIRE(parsed.ok);

            if (entry.has("stdout_contains")) {
                const auto result = runTool(args);

                CHECK(parsed.settings.help);
                CHECK(result.exitCode == 0);
                CHECK(result.out.find(entry["stdout_contains"].asString()) != std::string::npos);
                CHECK(result.err.empty());
            } else {
                CHECK_FALSE(parsed.settings.help);
            }

            if (entry.has("settings")) {
                for (const auto &[key, value] : entry["settings"].object) {
                    INFO("the setting " << key);
                    CHECK(settingOf(parsed.settings, key) == kitValueText(value));
                }
            }
        } else {
            const auto result = runTool(args);
            const auto line = firstLine(result.err);

            CHECK(result.exitCode == 2);

            // No arguments is not an error of the command line: the usage goes to stdout.
            if (args.empty()) {
                CHECK(result.out.find("spec_version nettest/1") != std::string::npos);
            } else {
                REQUIRE(line.rfind(prefix, 0) == 0);

                const auto text = line.substr(prefix.size());

                if (entry.has("error")) {
                    CHECK(text == entry["error"].asString());
                } else if (entry.has("error_prefix")) {
                    CHECK(text.rfind(entry["error_prefix"].asString(), 0) == 0);
                }
            }
        }

        ++checked;
    }

    std::string skippedList;

    for (const auto &command : skipped) {
        skippedList += "\n  " + command;
    }

    MESSAGE("cli-vectors.json: " << checked << " checked, " << skipped.size()
                                 << " skipped (isolate, legacy):" << skippedList);
    CHECK(checked + skipped.size() == vectors["vectors"].array.size());
    CHECK(checked >= 70);
}

TEST_CASE("NetTest: --help prints the usage with the version of the specification") {
    for (const auto &args : std::vector<std::vector<std::string>>{{"--help"}, {"p", ":7001", "-S", "100", "--help"}}) {
        const auto result = runTool(args);

        CHECK(result.exitCode == 0);
        CHECK(result.out.find("spec_version nettest/1") != std::string::npos);
        CHECK(result.err.empty());
    }
}

TEST_CASE("NetTest: no arguments print the usage and exit with 2") {
    const auto result = runTool({});

    CHECK(result.exitCode == 2);
    CHECK(result.out == usageText());
    CHECK(result.err.empty());
}

TEST_CASE("NetTest: the options of the native runtime and --legacy are not supported") {
    for (const auto &option : {"-Xmx512m", "-Xss1m", "-XX:+PrintGC", "--legacy"}) {
        const auto result = runTool({"p", ":7001", "-S", "100", option});

        INFO(option);
        CHECK(result.exitCode == 2);
        CHECK(firstLine(result.err) == std::string("NetTest: unsupported option: ") + option);
    }
}

TEST_CASE("NetTest: the options that the run does not implement yet are rejected after the parsing") {
    const std::vector<std::pair<std::vector<std::string>, std::string>> cases{
        {{"c", "127.0.0.1:7001", "-S", "10", "--metrics-port", "0"}, "--metrics-port"},
        {{"p", ":7001", "-S", "10", "--metrics-port", "0"}, "--metrics-port"},
    };

    for (const auto &[args, option] : cases) {
        const auto result = runTool(args);

        INFO(option);
        CHECK(result.exitCode == 2);
        CHECK(firstLine(result.err) == "NetTest: unsupported option: " + option);
    }
}

TEST_CASE("NetTest: the system properties keep the command line order") {
    const auto parsed = parseArguments({"p", ":7001", "-S", "5", "-Ddxscheme.nanoTime=false", "-Da=b=c"});

    REQUIRE(parsed.ok);
    REQUIRE(parsed.settings.properties.size() == 2);
    CHECK(parsed.settings.properties[0] == std::make_pair(std::string("dxscheme.nanoTime"), std::string("false")));
    CHECK(parsed.settings.properties[1] == std::make_pair(std::string("a"), std::string("b=c")));
    CHECK(endpointNanoTime(parsed.settings) == "false");
    CHECK_FALSE(isNanoTimeOn(parsed.settings));

    const auto plain = parseArguments({"p", ":7001", "-S", "5"});

    REQUIRE(plain.ok);
    CHECK(endpointNanoTime(plain.settings) == "true");
    CHECK(isNanoTimeOn(plain.settings));
}

TEST_CASE("NetTest kit: the first generated symbols are those of symbols-generated-10.txt") {
    const auto directory = kitDirectory();

    if (!directory) {
        return;
    }

    const auto expected = splitLines(kit::readText(*directory + "/symbols-generated-10.txt"));

    REQUIRE(expected.size() == 10);
    CHECK(generateSymbols(10) == expected);
}

TEST_CASE("NetTest kit: the generated symbols give the draws, samples and hashes of symbols-generated-vectors.json") {
    const auto directory = kitDirectory();

    if (!directory) {
        return;
    }

    const auto vectors = kit::readJson(*directory + "/symbols-generated-vectors.json");

    for (const auto &entry : nonEmpty(vectors["cases"])) {
        const auto total = entry["total"].asInteger();
        std::int64_t draws = 0;
        const auto symbols = generateSymbols(total, &draws);

        CAPTURE(total);
        REQUIRE(static_cast<std::int64_t>(symbols.size()) == total);
        CHECK(draws == entry["draws"].asInteger());

        for (const auto &sample : nonEmpty(entry["samples"])) {
            const auto index = sample["index"].asInteger();

            REQUIRE(index >= 0);
            REQUIRE(index < total);
            CHECK(symbols[static_cast<std::size_t>(index)] == sample["symbol"].asString());
        }

        // FNV-1a 64 of the symbols, each followed by a line feed.
        std::uint64_t hash = 0xCBF29CE484222325ULL;

        for (const auto &symbol : symbols) {
            for (const auto byte : symbol + '\n') {
                hash = (hash ^ static_cast<unsigned char>(byte)) * 0x100000001B3ULL;
            }
        }

        CHECK(hash == hexValue(entry["fnv1a64"]));
    }
}

TEST_CASE("NetTest: the generated symbols out of the range give an empty set") {
    CHECK(generateSymbols(0).empty());
    CHECK(generateSymbols(-1).empty());
    CHECK(generateSymbols(100'000'001).empty());
}

TEST_CASE("NetTest kit: the pools of the instances give the table and the first batch of pool-vectors.json") {
    const auto directory = kitDirectory();

    if (!directory) {
        return;
    }

    const auto vectors = kit::readJson(*directory + "/pool-vectors.json");

    for (const auto &instance : nonEmpty(vectors["instances"])) {
        const auto number = static_cast<std::int32_t>(instance["instance"].asInteger());
        const auto &events = instance["events"];
        const auto poolSize = static_cast<std::size_t>(events["pool_size"].asInteger());
        const auto symbols =
            std::make_shared<const std::vector<std::string>>(generateSymbols(static_cast<std::int64_t>(poolSize)));

        INFO("instance " << number);

        EventPool pool(number, symbols, EventKind::TRADE, false);

        CHECK(pool.seed() == hexValue(instance["seed"]));
        CHECK(pool.initialState() == hexValue(instance["initial_state"]));
        CHECK(pool.size() == poolSize);

        // The first draws make the rows 0 to 3: the price from the first draw of a row, the size from the second.
        const auto &draws = instance["first_draws"].array;
        auto state = pool.initialState();

        REQUIRE(draws.size() == 8);

        for (std::size_t at = 0; at < draws.size(); ++at) {
            CHECK(EventPool::nextDraw(state) == hexValue(draws[at]));
        }

        for (std::size_t row = 0; row < 4; ++row) {
            CHECK(pool.prices()[row] == static_cast<double>(100 + hexValue(draws[2 * row]) % 100'000) / 100.0);
            CHECK(pool.sizes()[row] == static_cast<double>(1 + hexValue(draws[2 * row + 1]) % 1000));
        }

        for (const auto &row : nonEmpty(instance["table_rows"])) {
            const auto at = static_cast<std::size_t>(row["row"].asInteger());

            CHECK(pool.prices()[at] == row["price"].asNumber());
            CHECK(pool.sizes()[at] == row["size"].asNumber());
        }

        // The table takes 2048 draws, then each event of the first batch takes one.
        state = pool.initialState();

        for (std::size_t at = 0; at < 2 * EventPool::TABLE_SIZE; ++at) {
            EventPool::nextDraw(state);
        }

        const auto &first = nonEmpty(events["first_events"]);
        constexpr std::int64_t epochNanos = 1'763'380'800'123'456'789LL;
        EventBatch batch;

        pool.fillAt(batch, first.size(), epochNanos);
        REQUIRE(batch.size() == first.size());

        for (std::size_t at = 0; at < first.size(); ++at) {
            const auto &expected = first[at];
            const auto draw = EventPool::nextDraw(state);
            const auto trade = std::dynamic_pointer_cast<Trade>(batch[at]);
            const auto symbolIndex = static_cast<std::size_t>(expected["symbol_index"].asInteger());
            const auto row = static_cast<std::size_t>(expected["row"].asInteger());

            INFO("event " << at);
            CHECK(draw == hexValue(expected["draw"]));
            CHECK((draw >> 32) % poolSize == symbolIndex);
            CHECK((draw >> 8) % EventPool::TABLE_SIZE == row);
            REQUIRE(trade != nullptr);
            CHECK(trade->getEventSymbol() == (*symbols)[symbolIndex]);
            CHECK(trade->getPrice() == expected["price"].asNumber());
            CHECK(trade->getSize() == expected["size"].asNumber());
            CHECK(trade->getTimeNanoPart() == 456'789);
            CHECK(trade->getTime() == 1'763'380'800'123LL);
            CHECK(trade->getTimeSequence() == packTimeSequence(epochNanos, static_cast<std::uint32_t>(at)));
        }
    }
}

TEST_CASE("NetTest: the pool of TimeAndSale events and the reuse of the pool events") {
    const auto symbols = std::make_shared<const std::vector<std::string>>(generateSymbols(3));
    constexpr std::int64_t epochNanos = 1'763'380'800'123'456'789LL;
    EventBatch batch;

    EventPool reusing(1, symbols, EventKind::TIME_AND_SALE, true);

    reusing.fillAt(batch, 20, epochNanos);
    REQUIRE(batch.size() == 20);

    for (std::size_t at = 0; at < batch.size(); ++at) {
        const auto sale = std::dynamic_pointer_cast<TimeAndSale>(batch[at]);

        REQUIRE(sale != nullptr);

        // 20 draws of 3 symbols: the events are the pool objects.
        CHECK((batch[at] == reusing.poolEvent(0) || batch[at] == reusing.poolEvent(1) ||
               batch[at] == reusing.poolEvent(2)));
        CHECK(sale->getTimeNanoPart() == 456'789);
        CHECK(sale->getTime() == 1'763'380'800'123LL);
    }

    // The sequence of the instance goes on between the batches.
    CHECK(reusing.nextSequence() == 20);

    EventPool copying(1, symbols, EventKind::TIME_AND_SALE, false);

    copying.fillAt(batch, 20, epochNanos);
    REQUIRE(batch.size() == 20);

    for (std::size_t at = 0; at < batch.size(); ++at) {
        const auto sale = std::dynamic_pointer_cast<TimeAndSale>(batch[at]);

        REQUIRE(sale != nullptr);
        CHECK((batch[at] != copying.poolEvent(0) && batch[at] != copying.poolEvent(1) &&
               batch[at] != copying.poolEvent(2)));
        CHECK(sale->getIndex() == packTimeSequence(epochNanos, static_cast<std::uint32_t>(at)));
        CHECK(sale->getEventSymbol().empty() == false);
    }

    // The pool events themselves keep their defaults: only the copies carry the stamp.
    const auto pooled = std::dynamic_pointer_cast<TimeAndSale>(copying.poolEvent(0));

    REQUIRE(pooled != nullptr);
    CHECK(pooled->getIndex() == 0);
}

TEST_CASE("NetTest kit: the time packing of pool-vectors.json through Trade and TimeAndSale") {
    const auto directory = kitDirectory();

    if (!directory) {
        return;
    }

    const auto vectors = kit::readJson(*directory + "/pool-vectors.json");

    for (const auto &entry : nonEmpty(vectors["pack_time_sequence"])) {
        const auto epochNanos = entry["epoch_ns"].asInteger();
        const auto sequence = static_cast<std::uint32_t>(entry["sequence"].asInteger());
        const auto packed = static_cast<std::int64_t>(hexValue(entry["packed"]));
        const auto nanoPart = static_cast<std::int32_t>(entry["nano_part"].asInteger());

        INFO("epoch_ns " << epochNanos << " sequence " << sequence);
        CHECK(packTimeSequence(epochNanos, sequence) == packed);
        CHECK(epochNanos / 1'000'000 == entry["time_ms"].asInteger());
        CHECK(epochNanos % 1'000'000 == nanoPart);

        Trade trade;

        trade.setTimeSequence(packed);
        trade.setTimeNanoPart(nanoPart);
        CHECK(trade.getTimeSequence() == packed);
        CHECK(trade.getTime() == entry["time_ms"].asInteger());
        CHECK(trade.getTimeNanoPart() == nanoPart);
        CHECK(trade.getSequence() == static_cast<std::int32_t>(sequence & EventPool::SEQUENCE_MASK));

        TimeAndSale sale;

        sale.setIndex(packed);
        sale.setTimeNanoPart(nanoPart);
        CHECK(sale.getIndex() == packed);
        CHECK(sale.getTime() == entry["time_ms"].asInteger());
        CHECK(sale.getTimeNanoPart() == nanoPart);
        CHECK(sale.getSequence() == static_cast<std::int32_t>(sequence & EventPool::SEQUENCE_MASK));
    }
}

TEST_CASE("NetTest: the symbols of an IPF file in file order, at most n") {
    if (!ipfReadable()) {
        return;
    }

    const auto result = readSymbolsFromFile(IPF_FILE, 3);

    REQUIRE(result.ok);
    CHECK(result.warning.empty());
    CHECK(result.error.empty());
    REQUIRE(result.symbols != nullptr);
    CHECK(*result.symbols == std::vector<std::string>{"TEST", "/TSTZ23", ".TEST231117C100"});

    const auto parsed = parseArguments({"p", ":7001", "-S", "2@" + IPF_FILE});

    REQUIRE(parsed.ok);

    const auto fromSettings = buildSymbolPool(parsed.settings);

    REQUIRE(fromSettings.ok);
    CHECK(*fromSettings.symbols == std::vector<std::string>{"TEST", "/TSTZ23"});

    const auto generated = buildSymbolPool(parseArguments({"p", ":7001", "-S", "20"}).settings);

    REQUIRE(generated.ok);
    CHECK(generated.symbols->size() == 20);
}

TEST_CASE("NetTest: an IPF file with fewer symbols than n gives the smaller set and a warning") {
    if (!ipfReadable()) {
        return;
    }

    const auto result = readSymbolsFromFile(IPF_FILE, 100'000);

    REQUIRE(result.ok);
    REQUIRE(result.symbols != nullptr);
    CHECK(result.symbols->size() >= 3);
    CHECK(result.symbols->size() < 100'000);
    CHECK(result.warning == "the IPF file '" + IPF_FILE + "' has " + std::to_string(result.symbols->size()) +
                                " symbols, fewer than the requested 100000");
}

TEST_CASE("NetTest: an IPF file without symbols and a missing file are errors") {
    if (!ipfReadable()) {
        return;
    }

    const auto empty = (std::filesystem::path(DXFCXX_TEST_TEMP_DIR) / "nettest-empty.ipf").string();

    {
        std::ofstream out(empty, std::ios::binary);

        out << "#STOCK::=TYPE,SYMBOL,DESCRIPTION\r\n##COMPLETE\r\n";
    }

    const auto noSymbols = readSymbolsFromFile(empty, 5);

    CHECK_FALSE(noSymbols.ok);
    CHECK(noSymbols.symbols == nullptr);
    CHECK(noSymbols.error == "the IPF file '" + empty + "' has no symbols");

    const auto missing = readSymbolsFromFile(IPF_FILE + ".missing", 5);

    CHECK_FALSE(missing.ok);
    CHECK(missing.symbols == nullptr);
    CHECK(missing.error.find(".missing") != std::string::npos);

    std::filesystem::remove(empty);
}
