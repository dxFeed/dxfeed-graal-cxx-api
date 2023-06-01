// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <cstdint>
#include <memory>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

namespace dxfcpp {

const EventTypeEnum &Quote::TYPE = EventTypeEnum::QUOTE;

std::int16_t Quote::getBidExchangeCode() const { return data_.bidExchangeCode; }

void Quote::setBidExchangeCode(char bidExchangeCode) { data_.bidExchangeCode = utf8to16(bidExchangeCode); }

void Quote::setBidExchangeCode(std::int16_t bidExchangeCode) { data_.bidExchangeCode = bidExchangeCode; }

std::int16_t Quote::getAskExchangeCode() const { return data_.askExchangeCode; }

void Quote::setAskExchangeCode(char askExchangeCode) { data_.askExchangeCode = utf8to16(askExchangeCode); }

void Quote::setAskExchangeCode(std::int16_t askExchangeCode) { data_.askExchangeCode = askExchangeCode; }

std::shared_ptr<Quote> Quote::fromGraalNative(void *graalNative) noexcept {
    if (!graalNative) {
        return {};
    }

    auto eventType = bit_cast<dxfg_event_type_t *>(graalNative);

    if (eventType->clazz != DXFG_EVENT_QUOTE) {
        return {};
    }

    try {
        auto graalQuote = bit_cast<dxfg_quote_t *>(graalNative);
        auto quote = std::make_shared<Quote>(dxfcpp::toString(graalQuote->market_event.event_symbol));

        quote->setEventTime(graalQuote->market_event.event_time);

        quote->data_ = {
            graalQuote->time_millis_sequence,
            graalQuote->time_nano_part,
            graalQuote->bid_time,
            graalQuote->bid_exchange_code,
            graalQuote->bid_price,
            graalQuote->bid_size,
            graalQuote->ask_time,
            graalQuote->ask_exchange_code,
            graalQuote->ask_price,
            graalQuote->ask_size,
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
        encodeChar(getBidExchangeCode()), getBidPrice(), getBidSize(), formatTimeStamp(getAskTime()),
        encodeChar(getAskExchangeCode()), getAskPrice(), getAskSize());
}

} // namespace dxfcpp