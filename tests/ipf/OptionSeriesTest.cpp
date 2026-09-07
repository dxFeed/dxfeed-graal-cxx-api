// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <doctest.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <bit>
#include <cstdint>
#include <limits>
#include <memory>
#include <set>
#include <unordered_set>

using namespace dxfcpp;

namespace {

using OptionSeriesType = OptionSeries<int>;

OptionSeriesType createSeries(double multiplier, double spc = 1.0) {
    OptionChainsBuilder<int> builder{};

    builder.setProduct("TEST");
    builder.setExpiration(1);
    builder.setMultiplier(multiplier);
    builder.setSPC(spc);
    builder.setCFI("OC");
    builder.setStrike(100.0);
    builder.addOption(std::make_shared<int>(1));

    auto chains = builder.getChains();
    auto series = chains.at("TEST").getSeries();

    REQUIRE(series.size() == 1);

    return *series.begin();
}

double nanWithPayload(std::uint64_t payload) {
    return std::bit_cast<double>(UINT64_C(0x7ff8000000000000) | payload);
}

} // namespace

TEST_CASE("OptionSeries equality should match Java Double.compare semantics") {
    const auto finite = createSeries(100.0);
    const auto adjacent = createSeries(std::nextafter(100.0, std::numeric_limits<double>::infinity()));
    const auto positiveZero = createSeries(0.0);
    const auto negativeZero = createSeries(-0.0);
    const auto positiveInfinity = createSeries(std::numeric_limits<double>::infinity());
    const auto firstNaN = createSeries(nanWithPayload(1));
    const auto secondNaN = createSeries(nanWithPayload(2));
    const auto positiveSpcZero = createSeries(1.0, 0.0);
    const auto negativeSpcZero = createSeries(1.0, -0.0);
    const auto firstSpcNaN = createSeries(1.0, nanWithPayload(1));
    const auto secondSpcNaN = createSeries(1.0, nanWithPayload(2));

    CHECK(finite == finite);
    CHECK(finite != adjacent);
    CHECK(positiveZero != negativeZero);
    CHECK(positiveInfinity == positiveInfinity);
    CHECK(firstNaN == secondNaN);
    CHECK(positiveSpcZero != negativeSpcZero);
    CHECK(firstSpcNaN == secondSpcNaN);
}

TEST_CASE("OptionSeries ordering should match Java Double.compare semantics") {
    const auto negativeZero = createSeries(-0.0);
    const auto positiveZero = createSeries(0.0);
    const auto finite = createSeries(100.0);
    const auto positiveInfinity = createSeries(std::numeric_limits<double>::infinity());
    const auto firstNaN = createSeries(nanWithPayload(1));
    const auto secondNaN = createSeries(nanWithPayload(2));
    const auto negativeSpcZero = createSeries(1.0, -0.0);
    const auto positiveSpcZero = createSeries(1.0, 0.0);
    const auto infiniteSpc = createSeries(1.0, std::numeric_limits<double>::infinity());
    const auto nanSpc = createSeries(1.0, nanWithPayload(1));

    CHECK(negativeZero < positiveZero);
    CHECK_FALSE(positiveZero < negativeZero);
    CHECK(finite < positiveInfinity);
    CHECK(positiveInfinity < firstNaN);
    CHECK_FALSE(firstNaN < secondNaN);
    CHECK_FALSE(secondNaN < firstNaN);
    CHECK(negativeSpcZero < positiveSpcZero);
    CHECK(infiniteSpc < nanSpc);
}

TEST_CASE("OptionSeries equality, ordering, and hashing should be consistent") {
    const auto zero = createSeries(0.0);
    const auto small = createSeries(1.0e-20);
    const auto firstNaN = createSeries(nanWithPayload(1));
    const auto secondNaN = createSeries(nanWithPayload(2));
    const auto firstSpcNaN = createSeries(1.0, nanWithPayload(1));
    const auto secondSpcNaN = createSeries(1.0, nanWithPayload(2));

    CHECK(zero != small);
    CHECK(zero < small);

    REQUIRE(firstNaN == secondNaN);
    CHECK(std::hash<OptionSeriesType>{}(firstNaN) == std::hash<OptionSeriesType>{}(secondNaN));

    REQUIRE(firstSpcNaN == secondSpcNaN);
    CHECK(std::hash<OptionSeriesType>{}(firstSpcNaN) == std::hash<OptionSeriesType>{}(secondSpcNaN));
}

TEST_CASE("OptionSeries should behave consistently in ordered and unordered containers") {
    const auto negativeZero = createSeries(-0.0);
    const auto positiveZero = createSeries(0.0);
    const auto firstNaN = createSeries(nanWithPayload(1));
    const auto secondNaN = createSeries(nanWithPayload(2));

    const std::set<OptionSeriesType> ordered{negativeZero, positiveZero, firstNaN, secondNaN};
    const std::unordered_set<OptionSeriesType> unordered{negativeZero, positiveZero, firstNaN, secondNaN};

    CHECK(ordered.size() == 3);
    CHECK(unordered.size() == 3);
}
