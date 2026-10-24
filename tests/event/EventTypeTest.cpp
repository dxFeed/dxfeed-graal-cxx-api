// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <doctest.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <vector>

using namespace std::literals;
using namespace dxfcpp;

TEST_CASE("SharedEntity::is<T>() should be true only for the type of the object and its bases") {
    const std::shared_ptr<SharedEntity> trade = std::make_shared<Trade>("AAPL");

    CHECK(trade->is<Trade>());
    CHECK(trade->is<TradeBase>());
    CHECK(trade->is<MarketEvent>());
    CHECK(trade->is<EventType>());
    CHECK(trade->is<LastingEvent>());
    CHECK_FALSE(trade->is<Quote>());
    CHECK_FALSE(trade->is<Order>());
    CHECK_FALSE(trade->is<IndexedEvent>());

    const std::shared_ptr<SharedEntity> order = std::make_shared<Order>("AAPL");

    CHECK(order->is<Order>());
    CHECK(order->is<OrderBase>());
    CHECK(order->is<IndexedEvent>());
    CHECK_FALSE(order->is<SpreadOrder>());
    CHECK_FALSE(order->is<LastingEvent>());

    const std::shared_ptr<SharedEntity> spreadOrder = std::make_shared<SpreadOrder>("AAPL");

    CHECK(spreadOrder->is<OrderBase>());
    CHECK_FALSE(spreadOrder->is<Order>());
}

namespace {
struct Flags {
    std::reference_wrapper<const EventTypeEnum> type;
    bool isLasting;
    bool isIndexed;
    bool isTimeSeries;
    bool isOnlyIndexed;
    bool isMarket;
};
} // namespace

TEST_CASE("EventTypeEnum flags should match the Java event classes") {
    // The Java classes of dxfeed-api: LastingEvent, IndexedEvent, TimeSeriesEvent and MarketEvent in the ancestry.
    const std::vector<Flags> expected{
        // type, isLasting, isIndexed, isTimeSeries, isOnlyIndexed, isMarket
        {EventTypeEnum::QUOTE, true, false, false, false, true},
        {EventTypeEnum::PROFILE, true, false, false, false, true},
        {EventTypeEnum::SUMMARY, true, false, false, false, true},
        {EventTypeEnum::GREEKS, true, true, true, false, true},
        {EventTypeEnum::CANDLE, true, true, true, false, false},
        {EventTypeEnum::UNDERLYING, true, true, true, false, true},
        {EventTypeEnum::THEO_PRICE, true, true, true, false, true},
        {EventTypeEnum::TRADE, true, false, false, false, true},
        {EventTypeEnum::TRADE_ETH, true, false, false, false, true},
        {EventTypeEnum::CONFIGURATION, true, false, false, false, false},
        {EventTypeEnum::MESSAGE, false, false, false, false, false},
        {EventTypeEnum::TIME_AND_SALE, false, true, true, false, true},
        {EventTypeEnum::ORDER, false, true, false, true, true},
        {EventTypeEnum::ANALYTIC_ORDER, false, true, false, true, true},
        {EventTypeEnum::OTC_MARKETS_ORDER, false, true, false, true, true},
        {EventTypeEnum::SPREAD_ORDER, false, true, false, true, true},
        {EventTypeEnum::SERIES, false, true, false, true, true},
        {EventTypeEnum::OPTION_SALE, false, true, false, true, true},
        {EventTypeEnum::ORDER_IMBALANCE, false, false, false, false, true},
        {EventTypeEnum::NUAM_ORDER, false, true, false, true, true},
        {EventTypeEnum::NUAM_TRADE, true, false, false, false, true},
        {EventTypeEnum::NUAM_TIME_AND_SALE, false, true, true, false, true},
        {EventTypeEnum::TEXT_MESSAGE, false, false, false, false, false},
        {EventTypeEnum::MARKET_MAKER, false, true, false, true, true},
        {EventTypeEnum::TEXT_CONFIGURATION, true, false, false, false, false},
    };

    for (const auto &flags : expected) {
        const auto &type = flags.type.get();

        CAPTURE(type.getName());

        CHECK(type.isLasting() == flags.isLasting);
        CHECK(type.isIndexed() == flags.isIndexed);
        CHECK(type.isTimeSeries() == flags.isTimeSeries);
        CHECK(type.isOnlyIndexed() == flags.isOnlyIndexed);
        CHECK(type.isMarket() == flags.isMarket);
    }

    // Every type of EventTypeEnum::ALL is in the table.
    for (const auto &type : EventTypeEnum::ALL) {
        CAPTURE(type.get().getName());

        CHECK(std::ranges::any_of(expected, [&type](const Flags &flags) {
            return flags.type.get().getId() == type.get().getId();
        }));
    }
}
