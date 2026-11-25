// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include <dxfeed_graal_cpp_api/api.hpp>

#include <doctest.h>

using namespace dxfcpp;
using namespace std::literals;

namespace {

std::shared_ptr<Order> priceLevel(std::int64_t time, const Side &side, double price, double size) {
    auto order = std::make_shared<Order>("AAPL");

    order->setSource(OrderSource::NTV);
    order->setTime(time);
    order->setOrderSide(side);
    order->setPrice(price);
    order->setSize(size);

    return order;
}

} // namespace

TEST_CASE("PriceLevelService::GRANULARITY_ATTRIBUTE_KEY is the candle symbol attribute of the granularity") {
    CHECK(PriceLevelService::GRANULARITY_ATTRIBUTE_KEY == "gr"sv);

    const auto symbol = CandleSymbol::valueOf("AAPL{=d,gr=s}");

    CHECK(symbol.getBaseSymbol() == "AAPL");
    CHECK(MarketEventSymbols::getAttributeStringByKey(
              symbol.toString(), std::string(PriceLevelService::GRANULARITY_ATTRIBUTE_KEY)) == "s");
}

TEST_CASE("PriceLevelChecker::validate accepts the price levels with the bid below the ask") {
    const std::vector orders{
        priceLevel(1'000, Side::BUY, 100.0, 10.0),
        priceLevel(1'000, Side::SELL, 101.0, 10.0),
        priceLevel(2'000, Side::BUY, 100.5, 5.0),
    };

    CHECK(PriceLevelChecker::validate(orders, std::chrono::minutes(1), false));
}

TEST_CASE("PriceLevelChecker::validate rejects the price levels with the bid above the ask") {
    // The orders of one time are checked when the orders of the next time begin.
    const std::vector orders{
        priceLevel(1'000, Side::BUY, 102.0, 10.0),
        priceLevel(1'000, Side::SELL, 101.0, 10.0),
        priceLevel(2'000, Side::BUY, 100.0, 5.0),
    };

    CHECK_FALSE(PriceLevelChecker::validate(orders, std::chrono::minutes(1), false));
}

TEST_CASE("PriceLevelChecker::validate rejects a time gap between the orders above the bound") {
    const std::vector orders{
        priceLevel(1'000, Side::BUY, 100.0, 10.0),
        priceLevel(1'000 + 2 * 60'000, Side::SELL, 101.0, 10.0),
    };

    CHECK_FALSE(PriceLevelChecker::validate(orders, std::chrono::minutes(1), false));
    CHECK(PriceLevelChecker::validate(orders, std::chrono::minutes(5), false));
}

TEST_CASE("PriceLevelChecker::validate rejects a null order") {
    const std::vector<std::shared_ptr<Order>> orders{priceLevel(1'000, Side::BUY, 100.0, 10.0), nullptr};

    CHECK_THROWS_AS(PriceLevelChecker::validate(orders, std::chrono::minutes(1), false), InvalidArgumentException);
}

TEST_CASE("PriceLevelService can be created and closed without a server, and close can be repeated") {
    // The RMI endpoint connects in the background, so nothing is requested from the address here.
    auto service = PriceLevelService::create("127.0.0.1:1");

    REQUIRE(service);

    service->close();
    service->close();
    service.reset();

    // The destructor closes a service that is not closed.
    CHECK_NOTHROW(PriceLevelService::create("127.0.0.1:1").reset());
}
