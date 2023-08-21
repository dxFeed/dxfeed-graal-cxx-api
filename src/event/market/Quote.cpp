// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <cstdint>
#include <memory>

#include "dxfeed_graal_cpp_api/event/market/Quote.hpp"
#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

namespace dxfcpp {

const EventTypeEnum &Quote::TYPE = EventTypeEnum::QUOTE;

std::int16_t Quote::getBidExchangeCode() const noexcept {
    return data_.bidExchangeCode;
}

void Quote::setBidExchangeCode(char bidExchangeCode) noexcept {
    data_.bidExchangeCode = utf8to16(bidExchangeCode);
}

void Quote::setBidExchangeCode(std::int16_t bidExchangeCode) noexcept {
    data_.bidExchangeCode = bidExchangeCode;
}

std::int16_t Quote::getAskExchangeCode() const noexcept {
    return data_.askExchangeCode;
}

void Quote::setAskExchangeCode(char askExchangeCode) noexcept {
    data_.askExchangeCode = utf8to16(askExchangeCode);
}

void Quote::setAskExchangeCode(std::int16_t askExchangeCode) noexcept {
    data_.askExchangeCode = askExchangeCode;
}

std::shared_ptr<Quote> Quote::fromGraal(void *graalNative) noexcept {
    if (!graalNative) {
        return {};
    }

    auto eventType = dxfcpp::bit_cast<dxfg_event_type_t *>(graalNative);

    if (eventType->clazz != dxfg_event_clazz_t::DXFG_EVENT_QUOTE) {
        return {};
    }

    try {
        auto graalQuote = dxfcpp::bit_cast<dxfg_quote_t *>(graalNative);
        auto quote = std::make_shared<Quote>(dxfcpp::toString(graalQuote->market_event.event_symbol));

        quote->setEventTime(graalQuote->market_event.event_time);

        quote->data_ = {
            .timeMillisSequence = graalQuote->time_millis_sequence,
            .timeNanoPart = graalQuote->time_nano_part,
            .bidTime = graalQuote->bid_time,
            .bidExchangeCode = graalQuote->bid_exchange_code,
            .bidPrice = graalQuote->bid_price,
            .bidSize = graalQuote->bid_size,
            .askTime = graalQuote->ask_time,
            .askExchangeCode = graalQuote->ask_exchange_code,
            .askPrice = graalQuote->ask_price,
            .askSize = graalQuote->ask_size,
        };

        return quote;
    } catch (...) {
        // TODO: error handling
        return {};
    }
}

std::string Quote::toString() const noexcept {
    return fmt::format(
        "Quote{{{}, eventTime={}, time={}, timeNanoPart={}, sequence={}, bidTime={}, bidExchange={}, bidPrice={}, "
        "bidSize={}, askTime={}, askExchange={}, askPrice={}, askSize={}}}",
        MarketEvent::getEventSymbol(), formatTimeStampWithMillis(MarketEvent::getEventTime()),
        formatTimeStampWithMillis(getTime()), getTimeNanoPart(), getSequence(), formatTimeStamp(getBidTime()),
        encodeChar(getBidExchangeCode()), dxfcpp::toString(getBidPrice()), dxfcpp::toString(getBidSize()),
        formatTimeStamp(getAskTime()), encodeChar(getAskExchangeCode()), dxfcpp::toString(getAskPrice()),
        dxfcpp::toString(getAskSize()));
}

void *Quote::toGraal() const noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug(toString() + "::toGraal()");
    }

    auto *graalQuote = new (std::nothrow) dxfg_quote_t{
        .market_event = {.event_type = {.clazz = dxfg_event_clazz_t::DXFG_EVENT_QUOTE},
                         .event_symbol = createCString(MarketEvent::getEventSymbol()),
                         .event_time = MarketEvent::getEventTime()},
        .time_millis_sequence = data_.timeMillisSequence,
        .time_nano_part = data_.timeNanoPart,
        .bid_time = data_.bidTime,
        .bid_exchange_code = data_.bidExchangeCode,
        .bid_price = data_.bidPrice,
        .bid_size = data_.bidSize,
        .ask_time = data_.askTime,
        .ask_exchange_code = data_.askExchangeCode,
        .ask_price = data_.askPrice,
        .ask_size = data_.askSize,
    };

    MarketEvent::fillGraalData(static_cast<void*>(graalQuote));

    if (!graalQuote) {
        // TODO: error handling
    }

    return static_cast<void *>(graalQuote);
}

void Quote::freeGraal(void *graalNative) noexcept {
    if (!graalNative) {
        return;
    }

    auto eventType = dxfcpp::bit_cast<dxfg_event_type_t *>(graalNative);

    if (eventType->clazz != dxfg_event_clazz_t::DXFG_EVENT_QUOTE) {
        return;
    }

    auto graalQuote = dxfcpp::bit_cast<dxfg_quote_t *>(graalNative);

    delete[] graalQuote->market_event.event_symbol;
    delete graalQuote;
}

} // namespace dxfcpp