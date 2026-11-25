// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <doctest.h>

#include <dxfeed_graal_cpp_api/api.hpp>

#include "../../tools/Tools/src/Args/Args.hpp"
#include "../../tools/Tools/src/Connect/ConnectTool.hpp"
#include "../../tools/Tools/src/Dump/DumpTool.hpp"
#include "../../tools/Tools/src/LatencyTest/LatencyTestTool.hpp"

#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>

using namespace dxfcpp::tools;

TEST_CASE("Tools: the quiet option") {
    REQUIRE(QuietArg::parse({"-q"}, 0).result);
    REQUIRE(QuietArg::parse({"--quiet"}, 0).result);
    REQUIRE(QuietArg::parse({"--quiet"}, 0).nextIndex == 1);
    REQUIRE_FALSE(QuietArg::parse({"--quiet2"}, 0).result);
    REQUIRE_FALSE(QuietArg::parse({"--quiet"}, 1).result);

    // The former spelling is still accepted but is not shown in the help.
    REQUIRE(QuietArg::parse({"--quite"}, 0).result);
    REQUIRE(QuietArg::parse({"--quite"}, 0).nextIndex == 1);
    REQUIRE(QuietArg::getFullName() == "-q, --quiet");
    REQUIRE(QuietArg::prepareHelp(2, 30, 120).find("quite") == std::string::npos);
}

TEST_CASE("Tools: Connect and Dump accept both spellings of the quiet option") {
    for (const std::string option : {"-q", "--quiet", "--quite"}) {
        CAPTURE(option);

        const auto connect = ConnectTool::Args::parse({"demo.dxfeed.com:7300", "Quote", "AAPL", option});

        REQUIRE_FALSE(connect.isError);
        REQUIRE(connect.result.isQuiet);

        const auto dump = DumpTool::Args::parse({"file.csv", "Quote", "AAPL", option});

        REQUIRE_FALSE(dump.isError);
        REQUIRE(dump.result.isQuiet);
    }

    REQUIRE_FALSE(ConnectTool::Args::parse({"demo.dxfeed.com:7300", "Quote", "AAPL"}).result.isQuiet);
    REQUIRE_FALSE(DumpTool::Args::parse({"file.csv", "Quote", "AAPL"}).result.isQuiet);
}

TEST_CASE("Tools: the exchange codes to ignore") {
    using Codes = std::unordered_set<std::int16_t>;

    REQUIRE(IgnoreExchangesArg::parseExchangeCodes("Q,Z").result == Codes{'Q', 'Z'});
    REQUIRE(IgnoreExchangesArg::parseExchangeCodes(" Q , Z ,, ").result == Codes{'Q', 'Z'});
    REQUIRE(IgnoreExchangesArg::parseExchangeCodes("").result.empty());

    const auto invalid = IgnoreExchangesArg::parseExchangeCodes("Q,XNAS");

    REQUIRE(invalid.isError);
    REQUIRE(invalid.errorString.find("XNAS") != std::string::npos);
}

TEST_CASE("Tools: LatencyTest --ignore-exchanges") {
    const auto parsed = LatencyTest::Args::parse(
        {"demo.dxfeed.com:7300", "Quote", "AAPL", "--ignore-exchanges", "Q,Z", "--interval", "5"});

    REQUIRE_FALSE(parsed.isError);
    REQUIRE(parsed.result.ignoredExchanges == std::unordered_set<std::int16_t>{'Q', 'Z'});
    REQUIRE(parsed.result.interval == 5);

    REQUIRE(LatencyTest::Args::parse({"demo.dxfeed.com:7300", "Quote", "AAPL"}).result.ignoredExchanges.empty());
    REQUIRE(LatencyTest::Args::parse({"demo.dxfeed.com:7300", "Quote", "AAPL", "--ignore-exchanges", "NYSE"}).isError);
}
