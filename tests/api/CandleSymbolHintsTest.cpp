// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// The hints of CandleSymbol::valueOf() about the likely mistakes in a candle symbol. The hints are reported once per
// string for the whole process, so every test case uses its own symbols.

#include <doctest.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include "../support/TestSupport.hpp"

#include <atomic>
#include <cstddef>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace dxfcpp;
using namespace dxfcpp::literals;
using dxfcpp::test::ErrorManagerGuard;

namespace {

std::vector<ErrorInfo> hints() {
    std::vector<ErrorInfo> result{};

    for (const auto &error : ErrorManager::getErrors()) {
        if (error.component == "CandleSymbol") {
            result.push_back(error);
        }
    }

    return result;
}

bool contains(const std::string &string, const std::string &part) {
    return string.find(part) != std::string::npos;
}

} // namespace

TEST_CASE("A candle symbol without a period gets one warning with the symbols that have a period") {
    ErrorManagerGuard guard{};

    CHECK(CandleSymbol::valueOf("HINT1").toString() == "HINT1");

    const auto errors = hints();

    REQUIRE(errors.size() == 1);
    CHECK(errors[0].level == ErrorInfo::Level::WARN);
    CHECK(contains(errors[0].message, "'HINT1' has no period"));
    CHECK(contains(errors[0].message, "'HINT1{=1m}'"));
    CHECK(contains(errors[0].message, "'HINT1{=d}'"));
    CHECK(contains(errors[0].message, "'HINT1{=t}'"));
}

TEST_CASE("The suggested period keeps the other attributes of the symbol") {
    ErrorManagerGuard guard{};

    CandleSymbol::valueOf("HINT15{price=bid}");

    const auto errors = hints();

    REQUIRE(errors.size() == 1);
    CHECK(contains(errors[0].message, "'HINT15{=1m,price=bid}'"));
}

TEST_CASE("The period type M gets a warning that a month is mo") {
    ErrorManagerGuard guard{};

    CHECK(CandleSymbol::valueOf("HINT2{=5M}").toString() == "HINT2{=5m}");
    CHECK(CandleSymbol::valueOf("HINT3{=M}").toString() == "HINT3{=m}");
    CHECK(CandleSymbol::valueOf("HINT18{=1.5E1M}").toString() == "HINT18{=15m}");

    const auto errors = hints();

    REQUIRE(errors.size() == 3);
    CHECK(errors[0].level == ErrorInfo::Level::WARN);
    CHECK(contains(errors[0].message, "'HINT2{=5M}'"));
    CHECK(contains(errors[0].message, "MINUTE ('5m')"));
    CHECK(contains(errors[0].message, "MONTH write '5mo'"));
    CHECK(contains(errors[1].message, "MINUTE ('m')"));
    CHECK(contains(errors[1].message, "MONTH write 'mo'"));
    CHECK(contains(errors[2].message, "MINUTE ('15m')"));
    CHECK(contains(errors[2].message, "MONTH write '15mo'"));
}

TEST_CASE("The period type PR gets a warning that PRICE_RENKO is pr") {
    ErrorManagerGuard guard{};

    CHECK(CandleSymbol::valueOf("HINT4{=2PR}").toString() == "HINT4{=2p}");
    CHECK(CandleSymbol::valueOf("HINT17{=Pr}").toString() == "HINT17{=p}");

    const auto errors = hints();

    REQUIRE(errors.size() == 2);
    CHECK(contains(errors[0].message, "PRICE ('2p')"));
    CHECK(contains(errors[0].message, "PRICE_RENKO write '2pr'"));
    CHECK(contains(errors[1].message, "PRICE_RENKO write 'pr'"));
}

TEST_CASE("The correct candle symbols get no warnings") {
    ErrorManagerGuard guard{};

    for (const auto *symbol : {"HINT5{=1m}", "HINT6{=d,price=bid}", "HINT7{=t}", "HINT8{=5mo}", "HINT9{=MIN}",
                               "HINT19{=MO}", "HINT20{=5S}", "HINT21{=pr}", "HINT22{=2Days}", "HINT23{=1e1m}"}) {
        CAPTURE(symbol);
        CandleSymbol::valueOf(symbol);
    }

    CHECK(hints().empty());
}

TEST_CASE("A candle symbol gets its warning once") {
    ErrorManagerGuard guard{};

    CandleSymbol::valueOf("HINT24");
    CandleSymbol::valueOf("HINT24");
    [[maybe_unused]] const auto literal = "HINT24"_c;

    CHECK(hints().size() == 1);
}

TEST_CASE("The literal _c gets the warnings") {
    ErrorManagerGuard guard{};

    [[maybe_unused]] const auto symbol = "HINT25"_c;

    REQUIRE(hints().size() == 1);
    CHECK(contains(hints()[0].message, "'HINT25' has no period"));
}

TEST_CASE("The overloads with attributes and the granularity of ORCS get no warnings") {
    ErrorManagerGuard guard{};

    CHECK(CandleSymbol::valueOf("HINT10", CandlePrice::BID).toString() == "HINT10{price=bid}");
    CHECK(CandleSymbol::valueOf("HINT26", {CandlePrice::ASK}).toString() == "HINT26{price=ask}");
    CHECK(CandleSymbol::valueOf("HINT11{gr=s}").toString() == "HINT11{gr=s}");

    CHECK(hints().empty());
}

TEST_CASE("A candle symbol that cannot be parsed throws without a warning") {
    ErrorManagerGuard guard{};

    CHECK_THROWS_AS(CandleSymbol::valueOf("HINT12{=e5m}"), InvalidArgumentException);
    CHECK_THROWS_AS(CandleSymbol::valueOf("HINT27{=5x}"), InvalidArgumentException);

    CHECK(hints().empty());
}

TEST_CASE("A handler that creates a candle symbol does not deadlock") {
    ErrorManagerGuard guard{};
    std::atomic<int> calls{};

    ErrorManager::setHandler([&calls](const ErrorInfo &error) {
        if (error.component == "CandleSymbol") {
            calls++;
            CandleSymbol::valueOf("HINT13B");
        }
    });

    CandleSymbol::valueOf("HINT13A");

    // A report from inside the handler goes to the journal only.
    CHECK(calls == 1);
    REQUIRE(hints().size() == 2);
    CHECK(contains(hints()[0].message, "'HINT13A'"));
    CHECK(contains(hints()[1].message, "'HINT13B'"));
}

TEST_CASE("A candle symbol created by several threads at once gets one warning") {
    ErrorManagerGuard guard{};
    std::vector<std::thread> threads{};

    for (int i = 0; i < 8; i++) {
        threads.emplace_back([] {
            for (int j = 0; j < 100; j++) {
                CandleSymbol::valueOf("HINT28");
            }
        });
    }

    for (auto &thread : threads) {
        thread.join();
    }

    CHECK(hints().size() == 1);
}

TEST_CASE_FIXTURE(test::LocalHubFixture, "The symbol of a received candle gets no warning") {
    ErrorManagerGuard guard{};

    // An overload with attributes: no warning when the symbol is created.
    const auto symbol = CandleSymbol::valueOf("HINT14", CandlePeriod::TICK);
    std::vector<std::shared_ptr<Candle>> received{};
    const auto subscription = feed->createSubscription(Candle::TYPE);

    REQUIRE(symbol.toString() == "HINT14");

    subscription->addEventListener([&received](const std::vector<std::shared_ptr<EventType>> &events) {
        for (const auto &event : events) {
            received.push_back(event->sharedAs<Candle>());
        }
    });
    subscription->addSymbols(symbol);

    publishAndProcess(std::make_shared<Candle>(symbol)->withClose(42.0).sharedAs<Candle>());

    REQUIRE(received.size() == 1);
    CHECK(received[0]->getEventSymbol().toString() == "HINT14");
    CHECK(hints().empty());

    // The received symbol was not counted as hinted: the user still gets the warning.
    CandleSymbol::valueOf("HINT14");

    CHECK(hints().size() == 1);

    subscription->close();
}
