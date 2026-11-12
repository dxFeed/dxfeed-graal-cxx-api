// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

// The events of tests/data/tapes/network.txt, the tape that the local server of the network tests sends.
// GenerateNetworkTape writes the tape from these events, event/NetworkTapeTest checks that the committed tape still
// matches them, and network/NetworkTest compares the events that come from the server with them.

#include <dxfeed_graal_cpp_api/api.hpp>

#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

namespace dxfcpp::test::network_tape {

/// The tape file, relative to tests/data.
inline constexpr auto TAPE_FILE = "tapes/network.txt";

inline constexpr auto ORDER_SYMBOL = "AAPL";
inline constexpr auto CANDLE_SYMBOL = "AAPL{=d}";

inline constexpr std::int64_t DAY = 86'400'000;

/// 2022-01-03 00:00:00 UTC, a Monday: the time of the oldest candle.
inline constexpr std::int64_t FIRST_CANDLE_TIME = 18'995 * DAY;

/// One candle per weekday, 156 weeks (2022-01-03 to 2024-12-27).
inline constexpr std::size_t CANDLE_COUNT = 780;

/// 2024-12-27 21:00:00 UTC: the time of the quotes and the orders.
inline constexpr std::int64_t EVENT_TIME = 20'084 * DAY + 21 * 3'600'000;

/// The time of the i-th candle, the oldest is 0: the weekdays from FIRST_CANDLE_TIME.
inline std::int64_t candleTime(std::size_t i) {
    return FIRST_CANDLE_TIME + static_cast<std::int64_t>(i / 5 * 7 + i % 5) * DAY;
}

inline std::vector<std::shared_ptr<Quote>> createQuotes() {
    std::vector<std::shared_ptr<Quote>> quotes{};
    const auto add = [&](const std::string &symbol, double bidPrice, double askPrice) {
        auto quote = std::make_shared<Quote>(symbol);

        quote->setBidTime(EVENT_TIME);
        quote->setBidExchangeCode('Q');
        quote->setBidPrice(bidPrice);
        quote->setBidSize(300);
        quote->setAskTime(EVENT_TIME + 1'000);
        quote->setAskExchangeCode('Q');
        quote->setAskPrice(askPrice);
        quote->setAskSize(500);
        quotes.push_back(quote);
    };

    add("AAPL", 254.25, 254.5);
    add("IBM", 223.75, 224.125);

    return quotes;
}

/**
 * The orders of AAPL on the NTV source, a snapshot from the highest index to the lowest: SNAPSHOT_BEGIN on the first,
 * SNAPSHOT_END on the last, whose index is 0 (QD accepts the end of an order snapshot only on the index 0).
 */
inline std::vector<std::shared_ptr<Order>> createOrders() {
    constexpr std::int64_t count = 8;
    std::vector<std::shared_ptr<Order>> orders{};

    for (std::int64_t index = count - 1; index >= 0; index--) {
        auto order = std::make_shared<Order>(ORDER_SYMBOL);
        const auto buy = index % 2 == 0;
        const auto priceStep = 0.25 * static_cast<double>(index);

        order->setIndex(index);
        order->setSource(OrderSource::NTV);
        order->setTime(EVENT_TIME + index * 1'000);
        order->setPrice(buy ? 254.0 - priceStep : 254.5 + priceStep);
        order->setSize(static_cast<double>(100 * (index + 1)));
        order->setOrderSide(buy ? Side::BUY : Side::SELL);
        order->setScope(Scope::ORDER);
        order->setExchangeCode('Q');

        if (index == count - 1) {
            order->setEventFlags(static_cast<std::int32_t>(EventFlag::SNAPSHOT_BEGIN.getFlag()));
        } else if (index == 0) {
            order->setEventFlags(static_cast<std::int32_t>(EventFlag::SNAPSHOT_END.getFlag()));
        }

        orders.push_back(order);
    }

    return orders;
}

/**
 * The daily candles of AAPL from the newest to the oldest: SNAPSHOT_BEGIN on the newest, SNAPSHOT_SNIP on the oldest.
 * Without the snip QD does not send the end of the snapshot to a subscription from a time older than the oldest candle.
 * The prices are multiples of 0.25, so the decimal format of the tape keeps them exact.
 */
inline std::vector<std::shared_ptr<Candle>> createCandles() {
    std::vector<std::shared_ptr<Candle>> candles{};

    for (auto i = CANDLE_COUNT; i-- > 0;) {
        auto candle = std::make_shared<Candle>(CandleSymbol::valueOf(CANDLE_SYMBOL));
        const auto n = static_cast<double>(i);
        const auto open = 100.0 + 0.25 * static_cast<double>(i % 97);
        const auto close = open + 0.5 * static_cast<double>(static_cast<int>(i % 5) - 2);
        const auto high = (open > close ? open : close) + 1.0;
        const auto low = (open < close ? open : close) - 1.0;

        candle->setTime(candleTime(i));
        candle->setCount(static_cast<std::int64_t>(100 + i));
        candle->setOpen(open);
        candle->setHigh(high);
        candle->setLow(low);
        candle->setClose(close);
        candle->setVolume(1'000.0 + 10.0 * n);
        candle->setVWAP((high + low) / 2.0);

        if (i == CANDLE_COUNT - 1) {
            candle->setEventFlags(static_cast<std::int32_t>(EventFlag::SNAPSHOT_BEGIN.getFlag()));
        } else if (i == 0) {
            candle->setEventFlags(static_cast<std::int32_t>(EventFlag::SNAPSHOT_SNIP.getFlag()));
        }

        candles.push_back(candle);
    }

    return candles;
}

/// All the events of the tape in the order of the file.
inline std::vector<std::shared_ptr<EventType>> createEvents() {
    std::vector<std::shared_ptr<EventType>> events{};

    for (const auto &quote : createQuotes()) {
        events.push_back(quote);
    }

    for (const auto &order : createOrders()) {
        events.push_back(order);
    }

    for (const auto &candle : createCandles()) {
        events.push_back(candle);
    }

    return events;
}

/**
 * Writes the events to a text tape without event times (the server reads the tape with `time=none`). The times of the
 * file are in the time zone of the process: generate the committed tape with `TZ=UTC`.
 */
inline void writeTape(const std::string &path) {
    std::remove(path.c_str());

    const auto endpoint = DXEndpoint::newBuilder()
                              ->withRole(DXEndpoint::Role::PUBLISHER)
                              ->withProperty(DXEndpoint::DXFEED_WILDCARD_ENABLE_PROPERTY, "true")
                              ->build();
    const auto events = createEvents();

    endpoint->connect("tape:" + path + "[format=text,time=none]");

    try {
        endpoint->getPublisher()->publishEvents(events.begin(), events.end());
    } catch (...) {
        endpoint->closeAndAwaitTermination();

        throw;
    }

    endpoint->awaitProcessed();
    endpoint->closeAndAwaitTermination();
}

} // namespace dxfcpp::test::network_tape
