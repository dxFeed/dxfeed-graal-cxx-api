// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// The tool against the kit of the NetTest specification (docs/wrapper-guide/nettest.md) from the archive of the Graal
// Native SDK: the parser gives the settings and the errors of cli-vectors.json.

#include <doctest.h>

#include <dxfeed_graal_cpp_api/api.hpp>

#include "../../tools/Tools/src/Args/Args.hpp"
#include "../../tools/Tools/src/NetTest/Args.hpp"
#include "../../tools/Tools/src/NetTest/NetTestTool.hpp"
#include "nettest/KitJson.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <optional>
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

// The tool has no isolate arguments and no deprecated publishing call: an entry that needs one of them is skipped, an\n// entry that needs their absence is checked.
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

} // namespace

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

    MESSAGE("cli-vectors.json: " << checked << " checked, " << skipped.size() << " skipped (isolate, legacy):"
                                 << skippedList);
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
    const std::string summary = (std::filesystem::path(DXFCXX_TEST_TEMP_DIR) / "nettest-guard.json").string();

    const std::vector<std::pair<std::vector<std::string>, std::string>> cases{
        {{"c", "127.0.0.1:7001", "-S", "10"}, "c"},
        {{"p", ":7001", "-S", "10", "--summary", summary}, "--summary"},
        {{"p", ":7001", "-S", "10", "--warmup", "1s"}, "--warmup"},
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
