// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// MDAPI-462: the util/ headers are public and compile on their own. Only they are included here (not api.hpp), so
// everything they declare must not depend on internal headers.

#include <dxfeed_graal_cpp_api/util/CmdArgsUtils.hpp>
#include <dxfeed_graal_cpp_api/util/Handler.hpp>
#include <dxfeed_graal_cpp_api/util/Time.hpp>
#include <dxfeed_graal_cpp_api/util/TimeFormat.hpp>
#include <dxfeed_graal_cpp_api/util/Timer.hpp>
#include <dxfeed_graal_cpp_api/util/Tristate.hpp>

#include <doctest.h>

#include <chrono>
#include <functional>
#include <future>
#include <set>
#include <string>

using namespace dxfcpp;
using namespace std::chrono_literals;

TEST_CASE("CmdArgsUtils parses the types and the symbols") {
    const auto [types, unknown] = CmdArgsUtils::parseTypes("Quote,Trade,NoSuchType");

    std::set<std::string> typeNames{};

    // By name: a lookup in the set needs the friend operator== of EventTypeEnum, which the DLL does not export.
    for (const auto &type : types) {
        typeNames.insert(type.get().getClassName());
    }

    CHECK(typeNames == std::set<std::string>{"Quote", "Trade"});
    REQUIRE(unknown.size() == 1);
    CHECK(unknown[0] == "NoSuchType");

    const auto symbols = CmdArgsUtils::parseSymbols("AAPL,IBM");

    std::set<std::string> names{};

    for (const auto &symbol : symbols) {
        names.insert(symbol.toStringUnderlying());
    }

    CHECK(names == std::set<std::string>{"AAPL", "IBM"});
}

TEST_CASE("TimeFormat formats the time") {
    CHECK(TimeFormat::GMT.format(0) == "0");
    CHECK(!TimeFormat::DEFAULT.format(1'700'000'000'123LL).empty());
    CHECK(!TimeFormat::DEFAULT_WITH_MILLIS.format(1'700'000'000'123LL).empty());
}

TEST_CASE("Timer runs the task once") {
    std::promise<int> done{};
    auto result = done.get_future();

    auto timer = Timer::runOnce(
        [&done] {
            done.set_value(42);
        },
        10ms);

    REQUIRE(result.wait_for(10s) == std::future_status::ready);
    CHECK(result.get() == 42);
    CHECK(timer != nullptr);
}

TEST_CASE("SimpleHandler calls the listeners") {
    SimpleHandler<void(int)> handler{};
    int sum = 0;

    handler += [&sum](int value) {
        sum += value;
    };

    handler(2);
    handler(3);

    CHECK(sum == 5);
}

TEST_CASE("Tristate has the three values") {
    CHECK(static_cast<int>(Tristate::FALSE) == 0);
    CHECK(static_cast<int>(Tristate::TRUE) == 1);
    CHECK(static_cast<int>(Tristate::NONE) == 2);
}

TEST_CASE("now() is the time in milliseconds since the epoch") {
    // 2023-11-14, as a lower bound.
    CHECK(now() > 1'700'000'000'000LL);
}
