// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <atomic>
#include <clocale>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include <dxfeed_graal_cpp_api/api.hpp>

#include <doctest.h>

#include "CandleAttributesJavaParity.hpp"

using namespace dxfcpp;
using namespace candle_java_parity;

namespace {

// Java Double.compare equality: NaN equals NaN, 0.0 and -0.0 differ.
bool sameDouble(double a, double b) {
    if (std::isnan(a) || std::isnan(b)) {
        return std::isnan(a) && std::isnan(b);
    }

    return math::exactlyEquals(a, b) && std::signbit(a) == std::signbit(b);
}

const CandleType &typeByName(const std::string &name) {
    for (const auto &type : CandleType::VALUES) {
        if (type.get().getName() == name) {
            return type.get();
        }
    }

    FAIL("Unknown candle type name: " << name);

    return CandleType::TICK;
}

} // namespace

TEST_CASE("CandlePeriod::parse should match Java") {
    for (const auto &c : PERIOD_PARSE) {
        const std::string input = c.input;

        CAPTURE(input);

        if (!c.ok) {
            CHECK_THROWS_AS(CandlePeriod::parse(c.input), InvalidArgumentException);

            continue;
        }

        std::optional<CandlePeriod> period{};

        CHECK_NOTHROW(period = CandlePeriod::parse(c.input));

        if (!period) {
            continue;
        }

        CHECK(sameDouble(period->getValue(), c.value));
        CHECK(period->getType().getName() == c.typeName);
        CHECK(period->toString() == c.str);
        CHECK(period->getPeriodIntervalMillis() == c.millis);
    }
}

TEST_CASE("CandlePeriod::valueOf should match Java string and interval") {
    for (const auto &c : PERIOD_VALUE_OF) {
        CAPTURE(c.value);
        const std::string typeName = c.typeName;

        CAPTURE(typeName);

        const auto period = CandlePeriod::valueOf(c.value, typeByName(c.typeName));

        CHECK(period.toString() == c.str);
        CHECK(period.getPeriodIntervalMillis() == c.millis);
    }
}

TEST_CASE("CandlePeriod equality should follow Double.compare") {
    const auto nan = std::numeric_limits<double>::quiet_NaN();

    CHECK(CandlePeriod::valueOf(0.0, CandleType::MINUTE) != CandlePeriod::valueOf(-0.0, CandleType::MINUTE));
    CHECK(CandlePeriod::valueOf(nan, CandleType::MINUTE) == CandlePeriod::valueOf(nan, CandleType::MINUTE));
    CHECK(std::hash<CandlePeriod>{}(CandlePeriod::valueOf(nan, CandleType::MINUTE)) ==
          std::hash<CandlePeriod>{}(CandlePeriod::valueOf(-nan, CandleType::MINUTE)));
    CHECK(CandlePeriod::parse("5.0m") == CandlePeriod::parse("5m"));
    CHECK(CandlePeriod::valueOf(1.0, CandleType::DAY) == CandlePeriod::DAY);
    CHECK(CandlePeriod::valueOf(1.0, CandleType::TICK) == CandlePeriod::TICK);
}

TEST_CASE("CandleType::parse should match Java") {
    for (const auto &c : TYPE_PARSE) {
        const std::string input = c.input;

        CAPTURE(input);

        if (std::string(c.typeName).empty()) {
            CHECK_THROWS_AS(CandleType::parse(c.input), InvalidArgumentException);

            continue;
        }

        CHECK(CandleType::parse(c.input).get().getName() == c.typeName);
    }
}

TEST_CASE("CandlePriceLevel::parse should match Java") {
    for (const auto &c : PRICE_LEVEL_PARSE) {
        const std::string input = c.input;

        CAPTURE(input);

        if (!c.ok) {
            CHECK_THROWS_AS(CandlePriceLevel::parse(c.input), InvalidArgumentException);

            continue;
        }

        std::optional<CandlePriceLevel> level{};

        CHECK_NOTHROW(level = CandlePriceLevel::parse(c.input));

        if (!level) {
            continue;
        }

        CHECK(level->toString() == c.str);
        CHECK((*level == CandlePriceLevel::DEFAULT) == c.isDefault);
    }
}

TEST_CASE("CandlePriceLevel::valueOf should match Java") {
    for (const auto &c : PRICE_LEVEL_VALUE_OF) {
        CAPTURE(c.value);

        if (std::string(c.str).empty()) {
            CHECK_THROWS_AS(CandlePriceLevel::valueOf(c.value), InvalidArgumentException);

            continue;
        }

        CHECK(CandlePriceLevel::valueOf(c.value).toString() == c.str);
    }
}

TEST_CASE("Candle attributes should not depend on the C locale") {
    const std::string saved = std::setlocale(LC_ALL, nullptr);
    const char *locale = nullptr;

    for (const char *name : {"de_DE.UTF-8", "de_DE.utf8", "de_DE", "de-DE", "German_Germany.1252"}) {
        if ((locale = std::setlocale(LC_ALL, name)) != nullptr) {
            break;
        }
    }

    if (locale == nullptr) {
        MESSAGE("No locale with a decimal comma is available, skipped");

        return;
    }

    CHECK(CandlePriceLevel::parse("1.5").toString() == "1.5");
    CHECK_THROWS_AS(CandlePriceLevel::parse("1,5"), InvalidArgumentException);
    CHECK(CandlePeriod::parse("0.5m").toString() == "0.5m");
    CHECK(CandlePeriod::valueOf(1.5, CandleType::MINUTE).toString() == "1.5m");

    std::setlocale(LC_ALL, saved.c_str());
}

TEST_CASE("CandlePeriod::toString should be safe to call from several threads") {
    const auto period = CandlePeriod::valueOf(5.0, CandleType::MINUTE);
    std::atomic<bool> go{false};
    std::vector<std::thread> threads{};
    std::vector<std::string> results(4);

    for (std::size_t i = 0; i < results.size(); i++) {
        threads.emplace_back([&, i] {
            while (!go.load()) {
                std::this_thread::yield();
            }

            results[i] = period.toString() + CandlePeriod::DAY.toString() + CandlePeriod::TICK.toString();
        });
    }

    go = true;

    for (auto &t : threads) {
        t.join();
    }

    for (const auto &r : results) {
        CHECK(r == "5mdt");
    }
}
