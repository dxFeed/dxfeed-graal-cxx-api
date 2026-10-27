// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// A release path (a destructor, a deleter, a DXFCPP_FINALLY body) must not throw: an exception of it goes to
// ErrorManager instead of terminating the process.

#include <doctest.h>
#include <dxfeed_graal_cpp_api/api.hpp>
#include <dxfeed_graal_cpp_api/internal/CallbackBarrier.hpp>

#include "../support/TestSupport.hpp"

#include <stdexcept>
#include <string>
#include <type_traits>

using namespace dxfcpp;
using dxfcpp::test::ErrorManagerGuard;

namespace {

void throwInFinally(bool &ran, std::size_t &firstLine, std::size_t &lastLine) {
    firstLine = __LINE__ + 1;
    DXFCPP_FINALLY([&ran] {
        ran = true;

        throw std::runtime_error("finally");
    });
    lastLine = __LINE__ - 1;
}

} // namespace

TEST_CASE("DXFCPP_FINALLY reports an exception of its body") {
    ErrorManagerGuard guard{};
    bool ran = false;
    std::size_t firstLine = 0;
    std::size_t lastLine = 0;

    throwInFinally(ran, firstLine, lastLine);

    const auto error = ErrorManager::getLastError();

    CHECK(ran);
    REQUIRE(error.has_value());
    CHECK(error->component == "DXFCPP_FINALLY");
    CHECK(error->message == "finally");

    // The location is a line of the statement: the first one on GCC and MSVC, the last one on Clang.
    const std::string file = "ReleaseTest.cpp:";

    REQUIRE(error->location.starts_with(file));

    const auto line = std::stoul(error->location.substr(file.size()));

    CHECK(line >= firstLine);
    CHECK(line <= lastLine);
}

TEST_CASE("DXFCPP_FINALLY runs its body once and reports nothing when it does not throw") {
    ErrorManagerGuard guard{};
    int runs = 0;

    {
        DXFCPP_FINALLY([&runs] {
            runs++;
        });
    }

    CHECK(runs == 1);
    CHECK(ErrorManager::getErrors().empty());
}

TEST_CASE("runRelease reports an exception of the body") {
    ErrorManagerGuard guard{};

    static_assert(noexcept(runRelease("Test", [] {
        throw std::runtime_error("x");
    })));

    const auto line = __LINE__ + 1;
    runRelease("Test", [] {
        throw std::runtime_error("release");
    });

    const auto error = ErrorManager::getLastError();

    REQUIRE(error.has_value());
    CHECK(error->component == "Test");
    CHECK(error->message == "release");
    CHECK(error->location == "ReleaseTest.cpp:" + std::to_string(line));
}

// Compile-time guards: the release paths stay noexcept. What their failure reports is checked by the cases above.
TEST_CASE("Release paths are noexcept") {
    static_assert(std::is_nothrow_destructible_v<DXFeedSubscription>);
    static_assert(std::is_nothrow_destructible_v<DXEndpoint>);
    static_assert(std::is_nothrow_destructible_v<IndexedEventTxModel<Order>>);
    static_assert(std::is_nothrow_destructible_v<IndexedTxModel<Order>>);
    static_assert(std::is_nothrow_destructible_v<TimeSeriesTxModel<Candle>>);
    static_assert(std::is_nothrow_destructible_v<MarketDepthModel<Order>>);
    static_assert(std::is_nothrow_destructible_v<SymbolWrapper>);
    static_assert(noexcept(std::declval<const SymbolWrapper &>().toGraal()));
    static_assert(noexcept(std::declval<const EventSourceWrapper &>().toGraal()));
    static_assert(noexcept(math::floorDiv<1000>(std::int64_t{})));
    static_assert(noexcept(math::floorMod<1000>(std::int64_t{})));
}
