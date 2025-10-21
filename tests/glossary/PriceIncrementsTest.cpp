// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <cmath>
#include <initializer_list>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <doctest.h>

using namespace std::literals;
using namespace dxfcpp;
using namespace doctest;

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> intDistrib(0);
std::uniform_int_distribution<> sideDistrib(0, 1);

std::int32_t checkPricePrecision(double value) {
    return PriceIncrements::valueOf(value)->getPricePrecision();
}

template <typename C1, typename C2> void assertCollectionsEquals(C1 &&c1, C2 &&c2) {
    REQUIRE(c1.size() == c2.size());

    auto it1 = c1.begin();
    auto it2 = c2.begin();

    for (; it1 != c1.end(); ++it1, ++it2) {
        if constexpr (std::is_floating_point_v<std::decay_t<decltype(*it1)>> ||
                      std::is_floating_point_v<std::decay_t<decltype(*it2)>>) {
            REQUIRE(math::equals(*it1, *it2));
        } else {
            REQUIRE(*it1 == *it2);
        }
    }
}

template <typename T, typename C2> void assertCollectionsEquals(std::initializer_list<T> c1, C2 &&c2) {
    assertCollectionsEquals(std::vector<T>{c1}, std::forward<C2>(c2));
}

template <typename C1, typename T> void assertCollectionsEquals(C1 &&c1, std::initializer_list<T> c2) {
    assertCollectionsEquals(std::forward<C1>(c1), std::vector<T>{c2});
}

template <typename T1, typename T2>
void assertCollectionsEquals(std::initializer_list<T1> c1, std::initializer_list<T2> c2) {
    assertCollectionsEquals(std::vector<T1>{c1}, std::vector<T2>{c2});
}

void roundNearest(const PriceIncrements::Ptr &pi, double price, double nearest) {
    REQUIRE(math::equals(nearest, pi->roundPrice(price)));
    REQUIRE(math::equals(-nearest, pi->roundPrice(-price)));
}

void roundDirection(const PriceIncrements::Ptr &pi, double price, double decrease, double nearest, double increase) {
    REQUIRE(math::equals(decrease, pi->roundPrice(price, -1)));
    REQUIRE(math::equals(nearest, pi->roundPrice(price, 0)));
    REQUIRE(math::equals(increase, pi->roundPrice(price, 1)));
}

void roundMode(const PriceIncrements::Ptr &pi, double price, double up, double down, double ceiling, double floor,
               double halfUp, double halfDown, double halfEven) {
    REQUIRE(math::equals(up, pi->roundPrice(price, RoundingMode::ROUNDING_MODE_UP)));
    REQUIRE(math::equals(down, pi->roundPrice(price, RoundingMode::ROUNDING_MODE_DOWN)));
    REQUIRE(math::equals(ceiling, pi->roundPrice(price, RoundingMode::ROUNDING_MODE_CEILING)));
    REQUIRE(math::equals(floor, pi->roundPrice(price, RoundingMode::ROUNDING_MODE_FLOOR)));
    REQUIRE(math::equals(halfUp, pi->roundPrice(price, RoundingMode::ROUNDING_MODE_HALF_UP)));
    REQUIRE(math::equals(halfDown, pi->roundPrice(price, RoundingMode::ROUNDING_MODE_HALF_DOWN)));
    REQUIRE(math::equals(halfEven, pi->roundPrice(price, RoundingMode::ROUNDING_MODE_HALF_EVEN)));
}

TEST_CASE("Test Precision") {
    auto getRandomPricePair = [] {
        const auto x = intDistrib(gen) % 100000000;

        return std::make_pair(static_cast<double>(x) / 100000000.0, static_cast<double>(x + 1) / 100000000.0);
    };

    auto pi = PriceIncrements::valueOf("0.00000001");

    REQUIRE(math::equals(pi->incrementPrice(0.60618514, 1), 0.60618515));

    for (std::size_t i = 1; i <= 100000; i++) {
        const auto [price0, price1] = getRandomPricePair();

        REQUIRE(math::equals(pi->incrementPrice(price0, 1), price1));
    }
}

TEST_CASE("Test Small Precision") {
    auto pi = PriceIncrements::valueOf("1E-18");

    REQUIRE(math::equals(pi->incrementPrice(0.0001, 1), 1.00000000000001E-4));
    REQUIRE(math::equals(pi->incrementPrice(1E-9, 1), 1.000000001E-9));
    REQUIRE(math::equals(pi->incrementPrice(1E-18, 1), 2E-18));
    REQUIRE(math::equals(pi->incrementPrice(1E-18, -1), 0));

    REQUIRE(math::equals(pi->incrementPrice(1E-9, -1), 9.99999999E-10));
    REQUIRE(math::equals(pi->incrementPrice(0, 1), 1E-18));
    REQUIRE(math::equals(pi->incrementPrice(0, -1), -1E-18));

    REQUIRE(math::equals(pi->incrementPrice(1E-15, -1), 9.99E-16));
    REQUIRE(math::equals(pi->incrementPrice(1E-16, -1), 9.9E-17));
    REQUIRE(math::equals(pi->incrementPrice(1E-17, -1), 9.0E-18));
}

TEST_CASE("Test Over Precisions") {
    auto pi = PriceIncrements::valueOf("1E-18");

    REQUIRE(math::equals(pi->incrementPrice(1, 1), 1));
    REQUIRE(math::equals(pi->incrementPrice(0.1, 1), 0.1));
    REQUIRE(math::equals(pi->incrementPrice(0.01, 1), 0.01));
    REQUIRE(math::equals(pi->incrementPrice(0.001, 1), 0.001));

    REQUIRE(math::equals(pi->incrementPrice(10, 1), 10));
    REQUIRE(math::equals(pi->incrementPrice(1E2, 1), 1E2));
    REQUIRE(math::equals(pi->incrementPrice(1E3, 1), 1E3));
    REQUIRE(math::equals(pi->incrementPrice(1E4, 1), 1E4));

    REQUIRE(math::equals(pi->incrementPrice(1, -1), 1));
    REQUIRE(math::equals(pi->incrementPrice(0.1, -1), 0.1));

    REQUIRE(math::equals(pi->incrementPrice(10, -1), 10));
    REQUIRE(math::equals(pi->incrementPrice(1E4, -1), 1E4));
}

TEST_CASE("Compute Precision") {
    REQUIRE(0 == checkPricePrecision(100000000.123456789));
    REQUIRE(4 == checkPricePrecision(100.12345678));
    REQUIRE(7 == checkPricePrecision(0.12345678912345678));
    REQUIRE(9 == checkPricePrecision(0.000100001));
    REQUIRE(4 == checkPricePrecision(0.000100000000000001));

    auto pi = PriceIncrements::valueOf("1E-10 1E-5; 0.01");

    REQUIRE(2 == pi->getPricePrecision(1.0000000000100001E-5));
    REQUIRE(10 == pi->getPricePrecision(1.00000000001E-5));

    for (int i = 0; i <= 18; i++) {
        REQUIRE(i == checkPricePrecision(std::pow(10.0, -i)));
    }
}

TEST_CASE("Test Crypto Price Increments") {
    auto pi = PriceIncrements::valueOf("1E-18");

    REQUIRE(pi->getText() == "1E-18");
    assertCollectionsEquals({1E-18}, pi->getPriceIncrements());

    REQUIRE(pi->getText() == PriceIncrements::valueOf(1E-18)->getText());
    REQUIRE(pi->getText() == PriceIncrements::valueOf(std::vector{1E-18})->getText());

    REQUIRE(pi->getPricePrecision() == 18);
    REQUIRE(pi->getPricePrecision(0.12345) == 18);

    REQUIRE(math::equals(pi->getPriceIncrement(math::NaN), 1E-18));

    REQUIRE(math::equals(pi->getPriceIncrement(1E-10), 1E-18));
    REQUIRE(math::equals(pi->getPriceIncrement(1E10), 1E-18));

    REQUIRE(math::equals(pi->roundPrice(1.1E-15, 1), 1.1E-15));
    REQUIRE(math::equals(pi->roundPrice(1.01E-15, 1), 1.01E-15));
    REQUIRE(math::equals(pi->roundPrice(1.001E-15, 1), 1.001E-15));
    REQUIRE(math::equals(pi->roundPrice(1.0001E-15, 1), 1.001E-15));
    REQUIRE(math::equals(pi->roundPrice(1.00012345E-15, 1), 1.001E-15));

    REQUIRE(math::equals(pi->roundPrice(1.1E-15, -1), 1.1E-15));
    REQUIRE(math::equals(pi->roundPrice(1.01E-15, -1), 1.01E-15));
    REQUIRE(math::equals(pi->roundPrice(1.001E-15, -1), 1.001E-15));
    REQUIRE(math::equals(pi->roundPrice(1.0001E-15, -1), 1E-15));
    REQUIRE(math::equals(pi->roundPrice(1.00012345E-15, -1), 1E-15));

    REQUIRE(math::equals(pi->incrementPrice(1E-15, 1), 1.001E-15));
    REQUIRE(math::equals(pi->incrementPrice(1E-15, -1), 9.99E-16));

    REQUIRE(math::equals(pi->incrementPrice(1, 1), 1));
    REQUIRE(math::equals(pi->incrementPrice(0.1, 1), 0.1));
    REQUIRE(math::equals(pi->incrementPrice(0.01, 1), 0.01));
    REQUIRE(math::equals(pi->incrementPrice(0.001, 1), 0.001));
    REQUIRE(math::equals(pi->incrementPrice(1E-4, 1), 1.00000000000001E-4));
    REQUIRE(math::equals(pi->incrementPrice(1E-5, 1), 1.0000000000001E-5));
    REQUIRE(math::equals(pi->incrementPrice(1E-6, 1), 1.000000000001E-6));
    REQUIRE(math::equals(pi->incrementPrice(1E-7, 1), 1.00000000001E-7));
    REQUIRE(math::equals(pi->incrementPrice(1E-8, 1), 1.0000000001E-8));
    REQUIRE(math::equals(pi->incrementPrice(1E-9, 1), 1.000000001E-9));
    REQUIRE(math::equals(pi->incrementPrice(1E-10, 1), 1.00000001E-10));
    REQUIRE(math::equals(pi->incrementPrice(1E-11, 1), 1.0000001E-11));
    REQUIRE(math::equals(pi->incrementPrice(1E-12, 1), 1.000001E-12));
    REQUIRE(math::equals(pi->incrementPrice(1E-13, 1), 1.00001E-13));
    REQUIRE(math::equals(pi->incrementPrice(1E-14, 1), 1.0001E-14));
    REQUIRE(math::equals(pi->incrementPrice(1E-15, 1), 1.001E-15));
    REQUIRE(math::equals(pi->incrementPrice(1E-16, 1), 1.01E-16));
    REQUIRE(math::equals(pi->incrementPrice(1E-17, 1), 1.1E-17));
    REQUIRE(math::equals(pi->incrementPrice(1E-18, 1), 2E-18));

    REQUIRE(math::equals(pi->incrementPrice(1.23456E-10, 1), 1.23456001E-10));
    REQUIRE(math::equals(pi->incrementPrice(1.23456789E-15, 1), 1.236E-15));
    REQUIRE(math::equals(pi->incrementPrice(1.23456788E-10, 1), 1.23456789E-10));
}

TEST_CASE("Test Logic") {
    auto pi = PriceIncrements::valueOf("0.01");

    REQUIRE(pi->getText() == "0.01");
    assertCollectionsEquals({0.01}, pi->getPriceIncrements());
    REQUIRE(pi->getText() == PriceIncrements::valueOf(0.01)->getText());
    REQUIRE(pi->getText() == PriceIncrements::valueOf({0.01})->getText());
    REQUIRE(math::equals(pi->getPriceIncrement(0.01), 0.01));
    REQUIRE(math::equals(pi->getPriceIncrement(math::NaN), 0.01));
    REQUIRE(math::equals(pi->getPriceIncrement(0.1234), 0.01));
    REQUIRE(math::equals(pi->getPriceIncrement(-0.1234), 0.01));
    REQUIRE(math::equals(pi->getPriceIncrement(1234), 0.01));
    REQUIRE(math::equals(pi->getPriceIncrement(-1234), 0.01));
    REQUIRE(pi->getPricePrecision() == 2);
    REQUIRE(pi->getPricePrecision(math::NaN) == 2);
    REQUIRE(pi->getPricePrecision(0.1234) == 2);
    REQUIRE(pi->getPricePrecision(-0.1234) == 2);
    REQUIRE(pi->getPricePrecision(1234) == 2);
    REQUIRE(pi->getPricePrecision(-1234) == 2);
    REQUIRE(math::equals(pi->roundPrice(0.1234), 0.12));
    REQUIRE(math::equals(pi->roundPrice(0.1234, 1), 0.13));
    REQUIRE(math::equals(pi->roundPrice(0.1234, -1), 0.12));
    REQUIRE(math::equals(pi->roundPrice(-0.1234), -0.12));
    REQUIRE(math::equals(pi->roundPrice(-0.1234, 1), -0.12));
    REQUIRE(math::equals(pi->roundPrice(-0.1234, -1), -0.13));
    REQUIRE(math::equals(pi->incrementPrice(0.1234, 1), 0.13));
    REQUIRE(math::equals(pi->incrementPrice(0.1234, -1), 0.11));
    REQUIRE(math::equals(pi->incrementPrice(-0.1234, 1), -0.11));
    REQUIRE(math::equals(pi->incrementPrice(-0.1234, -1), -0.13));

    pi = PriceIncrements::valueOf("0.0001 1; 0.01");
    REQUIRE(pi->getText() == "0.0001 1; 0.01");
    assertCollectionsEquals({0.0001, 1.0, 0.01}, pi->getPriceIncrements());
    REQUIRE(pi->getText() == PriceIncrements::valueOf({0.0001, 1.0, 0.01})->getText());
    REQUIRE(math::equals(pi->getPriceIncrement(), 0.0001));
    REQUIRE(math::equals(pi->getPriceIncrement(math::NaN), 0.0001));
    REQUIRE(math::equals(pi->getPriceIncrement(0.1234), 0.0001));
    REQUIRE(math::equals(pi->getPriceIncrement(-0.1234), 0.0001));
    REQUIRE(math::equals(pi->getPriceIncrement(1, -1), 0.0001));
    REQUIRE(math::equals(pi->getPriceIncrement(1, 1), 0.01));
    REQUIRE(math::equals(pi->getPriceIncrement(1234), 0.01));
    REQUIRE(math::equals(pi->getPriceIncrement(-1234), 0.01));
    REQUIRE(pi->getPricePrecision() == 4);
    REQUIRE(pi->getPricePrecision(math::NaN) == 4);
    REQUIRE(pi->getPricePrecision(0.1234) == 4);
    REQUIRE(pi->getPricePrecision(-0.1234) == 4);
    REQUIRE(pi->getPricePrecision(1) == 4);
    REQUIRE(pi->getPricePrecision(1234) == 2);
    REQUIRE(pi->getPricePrecision(-1234) == 2);
    REQUIRE(math::equals(pi->roundPrice(0.1234), 0.1234));
    REQUIRE(math::equals(pi->roundPrice(0.123456), 0.1235));
    REQUIRE(math::equals(pi->roundPrice(0.123456, 1), 0.1235));
    REQUIRE(math::equals(pi->roundPrice(0.123456, -1), 0.1234));
    REQUIRE(math::equals(pi->roundPrice(-0.1234), -0.1234));
    REQUIRE(math::equals(pi->roundPrice(-0.123456), -0.1235));
    REQUIRE(math::equals(pi->roundPrice(-0.123456, 1), -0.1234));
    REQUIRE(math::equals(pi->roundPrice(-0.123456, -1), -0.1235));
    REQUIRE(math::equals(pi->roundPrice(7.1234), 7.12));
    REQUIRE(math::equals(pi->roundPrice(7.123456), 7.12));
    REQUIRE(math::equals(pi->roundPrice(7.123456, 1), 7.13));
    REQUIRE(math::equals(pi->roundPrice(7.123456, -1), 7.12));
    REQUIRE(math::equals(pi->roundPrice(-7.1234), -7.12));
    REQUIRE(math::equals(pi->roundPrice(-7.123456), -7.12));
    REQUIRE(math::equals(pi->roundPrice(-7.123456, 1), -7.12));
    REQUIRE(math::equals(pi->roundPrice(-7.123456, -1), -7.13));
    REQUIRE(math::equals(pi->incrementPrice(0.1234, 1), 0.1235));
    REQUIRE(math::equals(pi->incrementPrice(0.1234, -1), 0.1233));
    REQUIRE(math::equals(pi->incrementPrice(0.123456, 1), 0.1236));
    REQUIRE(math::equals(pi->incrementPrice(0.123456, -1), 0.1234));
    REQUIRE(math::equals(pi->incrementPrice(7.123456, 1), 7.13));
    REQUIRE(math::equals(pi->incrementPrice(7.123456, -1), 7.11));
    REQUIRE(math::equals(pi->incrementPrice(0.123456, 1, 7), 7.12));
    REQUIRE(math::equals(pi->incrementPrice(0.123456, -1, 7), -6.88));
    REQUIRE(math::equals(pi->incrementPrice(-7.123456, 1, 7), -0.1235));
    REQUIRE(math::equals(pi->incrementPrice(7.123456, -1, 7), 0.1235));
}

TEST_CASE("Test Round") {
    auto pi = PriceIncrements::valueOf("0.01");

    roundNearest(pi, 0.12500005, 0.13);
    roundNearest(pi, 0.12500000005, 0.12);
    roundNearest(pi, 0.12499995, 0.12);
    roundNearest(pi, 0.12499999995, 0.12);
    roundNearest(pi, 0.11500005, 0.12);
    roundNearest(pi, 0.11500000005, 0.12);
    roundNearest(pi, 0.11499995, 0.11);
    roundNearest(pi, 0.11499999995, 0.12);

    roundDirection(pi, 0.12000005, 0.12, 0.12, 0.13);
    roundDirection(pi, 0.12000000005, 0.12, 0.12, 0.12);
    roundDirection(pi, 0.11999995, 0.11, 0.12, 0.12);
    roundDirection(pi, 0.11999999995, 0.12, 0.12, 0.12);
    roundDirection(pi, -0.12000005, -0.13, -0.12, -0.12);
    roundDirection(pi, -0.12000000005, -0.12, -0.12, -0.12);
    roundDirection(pi, -0.11999995, -0.12, -0.12, -0.11);
    roundDirection(pi, -0.11999999995, -0.12, -0.12, -0.12);

    roundMode(pi, 0.12000005, 0.13, 0.12, 0.13, 0.12, 0.12, 0.12, 0.12);
    roundMode(pi, 0.12000000005, 0.12, 0.12, 0.12, 0.12, 0.12, 0.12, 0.12);
    roundMode(pi, 0.11999995, 0.12, 0.11, 0.12, 0.11, 0.12, 0.12, 0.12);
    roundMode(pi, 0.11999999995, 0.12, 0.12, 0.12, 0.12, 0.12, 0.12, 0.12);
    roundMode(pi, -0.12000005, -0.13, -0.12, -0.12, -0.13, -0.12, -0.12, -0.12);
    roundMode(pi, -0.12000000005, -0.12, -0.12, -0.12, -0.12, -0.12, -0.12, -0.12);
    roundMode(pi, -0.11999995, -0.12, -0.11, -0.11, -0.12, -0.12, -0.12, -0.12);
    roundMode(pi, -0.11999999995, -0.12, -0.12, -0.12, -0.12, -0.12, -0.12, -0.12);

    roundMode(pi, 0.12500005, 0.13, 0.12, 0.13, 0.12, 0.13, 0.13, 0.13);
    roundMode(pi, 0.12500000005, 0.13, 0.12, 0.13, 0.12, 0.13, 0.12, 0.12);
    roundMode(pi, 0.12499995, 0.13, 0.12, 0.13, 0.12, 0.12, 0.12, 0.12);
    roundMode(pi, 0.12499999995, 0.13, 0.12, 0.13, 0.12, 0.13, 0.12, 0.12);
    roundMode(pi, 0.11500005, 0.12, 0.11, 0.12, 0.11, 0.12, 0.12, 0.12);
    roundMode(pi, 0.11500000005, 0.12, 0.11, 0.12, 0.11, 0.12, 0.11, 0.12);
    roundMode(pi, 0.11499995, 0.12, 0.11, 0.12, 0.11, 0.11, 0.11, 0.11);
    roundMode(pi, 0.11499999995, 0.12, 0.11, 0.12, 0.11, 0.12, 0.11, 0.12);
    roundMode(pi, -0.12500005, -0.13, -0.12, -0.12, -0.13, -0.13, -0.13, -0.13);
    roundMode(pi, -0.12500000005, -0.13, -0.12, -0.12, -0.13, -0.13, -0.12, -0.12);
    roundMode(pi, -0.12499995, -0.13, -0.12, -0.12, -0.13, -0.12, -0.12, -0.12);
    roundMode(pi, -0.12499999995, -0.13, -0.12, -0.12, -0.13, -0.13, -0.12, -0.12);
    roundMode(pi, -0.11500005, -0.12, -0.11, -0.11, -0.12, -0.12, -0.12, -0.12);
    roundMode(pi, -0.11500000005, -0.12, -0.11, -0.11, -0.12, -0.12, -0.11, -0.12);
    roundMode(pi, -0.11499995, -0.12, -0.11, -0.11, -0.12, -0.11, -0.11, -0.11);
    roundMode(pi, -0.11499999995, -0.12, -0.11, -0.11, -0.12, -0.12, -0.11, -0.12);

    REQUIRE(math::equals(0.12, pi->roundPrice(0.12, RoundingMode::ROUNDING_MODE_UNNECESSARY)));
    REQUIRE(math::equals(-0.12, pi->roundPrice(-0.12, RoundingMode::ROUNDING_MODE_UNNECESSARY)));

    REQUIRE_THROWS_AS(pi->roundPrice(0.1234, RoundingMode::ROUNDING_MODE_UNNECESSARY), JavaException);
    REQUIRE_THROWS_AS(pi->roundPrice(-0.1234, RoundingMode::ROUNDING_MODE_UNNECESSARY), JavaException);
}