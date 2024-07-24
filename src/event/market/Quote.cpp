// Copyright (c) 2024 Devexperts LLC.
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

DXFCPP_BEGIN_NAMESPACE

const EventTypeEnum &Quote::TYPE = EventTypeEnum::QUOTE;

std::int16_t Quote::getBidExchangeCode() const noexcept {
    return data_.bidExchangeCode;
}

std::string Quote::getBidExchangeCodeString() const noexcept {
    // TODO: cache [EN-8231]

    return utf16toUtf8String(data_.bidExchangeCode);
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

std::string Quote::getAskExchangeCodeString() const noexcept {
    // TODO: cache [EN-8231]

    return utf16toUtf8String(data_.askExchangeCode);
}

void Quote::setAskExchangeCode(char askExchangeCode) noexcept {
    data_.askExchangeCode = utf8to16(askExchangeCode);
}

void Quote::setAskExchangeCode(std::int16_t askExchangeCode) noexcept {
    data_.askExchangeCode = askExchangeCode;
}

void Quote::fillData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    MarketEvent::fillData(graalNative);

    auto graalQuote = static_cast<dxfg_quote_t *>(graalNative);

    data_ = {
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
}

void Quote::fillGraalData(void *graalNative) const noexcept {
    if (graalNative == nullptr) {
        return;
    }

    MarketEvent::fillGraalData(graalNative);

    auto graalQuote = static_cast<dxfg_quote_t *>(graalNative);

    graalQuote->market_event.event_type.clazz = dxfg_event_clazz_t::DXFG_EVENT_QUOTE;
    graalQuote->time_millis_sequence = data_.timeMillisSequence;
    graalQuote->time_nano_part = data_.timeNanoPart;
    graalQuote->bid_time = data_.bidTime;
    graalQuote->bid_exchange_code = data_.bidExchangeCode;
    graalQuote->bid_price = data_.bidPrice;
    graalQuote->bid_size = data_.bidSize;
    graalQuote->ask_time = data_.askTime;
    graalQuote->ask_exchange_code = data_.askExchangeCode;
    graalQuote->ask_price = data_.askPrice;
    graalQuote->ask_size = data_.askSize;
}

std::string Quote::toString() const {
    return fmt::format(
        "Quote{{{}, eventTime={}, time={}, timeNanoPart={}, sequence={}, bidTime={}, bidExchange={}, bidPrice={}, "
        "bidSize={}, askTime={}, askExchange={}, askPrice={}, askSize={}}}",
        MarketEvent::getEventSymbol(), TimeFormat::DEFAULT_WITH_MILLIS.format(MarketEvent::getEventTime()),
        TimeFormat::DEFAULT_WITH_MILLIS.format(getTime()), getTimeNanoPart(), getSequence(),
        TimeFormat::DEFAULT.format(getBidTime()), encodeChar(getBidExchangeCode()), dxfcpp::toString(getBidPrice()),
        dxfcpp::toString(getBidSize()), TimeFormat::DEFAULT.format(getAskTime()), encodeChar(getAskExchangeCode()),
        dxfcpp::toString(getAskPrice()), dxfcpp::toString(getAskSize()));
}

std::shared_ptr<Quote> Quote::fromGraal(void *graalNative) {
    if (!graalNative) {
        throw InvalidArgumentException("Unable to create Quote. The `graalNative` parameter is nullptr");
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_QUOTE) {
        throw InvalidArgumentException(
            fmt::format("Unable to create Quote. Wrong event class {}! Expected: {}.",
                        std::to_string(static_cast<int>(static_cast<dxfg_event_type_t *>(graalNative)->clazz)),
                        std::to_string(static_cast<int>(dxfg_event_clazz_t::DXFG_EVENT_QUOTE))));
    }

    auto quote = std::make_shared<Quote>();

    quote->fillData(graalNative);

    return quote;
}

void *Quote::toGraal() const {
    if constexpr (Debugger::isDebug) {
        Debugger::debug(toString() + "::toGraal()");
    }

    auto *graalQuote = new dxfg_quote_t{};

    fillGraalData(static_cast<void *>(graalQuote));

    return static_cast<void *>(graalQuote);
}

void Quote::freeGraal(void *graalNative) {
    if (!graalNative) {
        return;
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != dxfg_event_clazz_t::DXFG_EVENT_QUOTE) {
        throw InvalidArgumentException(
            fmt::format("Unable to free Quote's Graal data. Wrong event class {}! Expected: {}.",
                        std::to_string(static_cast<int>(static_cast<dxfg_event_type_t *>(graalNative)->clazz)),
                        std::to_string(static_cast<int>(dxfg_event_clazz_t::DXFG_EVENT_QUOTE))));
    }

    auto graalQuote = static_cast<dxfg_quote_t *>(graalNative);

    MarketEvent::freeGraalData(graalNative);

    delete graalQuote;
}

DXFCPP_END_NAMESPACE