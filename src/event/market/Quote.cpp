// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/market/Quote.hpp"

#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../../../include/dxfeed_graal_cpp_api/internal/TimeFormat.hpp"

#include <cstdint>
#include <dxfg_api.h>
#include <fmt/format.h>
#include <memory>

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

Quote &Quote::withBidExchangeCode(char bidExchangeCode) noexcept {
    setBidExchangeCode(bidExchangeCode);

    return *this;
}

void Quote::setBidExchangeCode(std::int16_t bidExchangeCode) noexcept {
    data_.bidExchangeCode = bidExchangeCode;
}

Quote &Quote::withBidExchangeCode(std::int16_t bidExchangeCode) noexcept {
    setBidExchangeCode(bidExchangeCode);

    return *this;
}

double Quote::getBidPrice() const noexcept {
    return data_.bidPrice;
}

void Quote::setBidPrice(double bidPrice) noexcept {
    data_.bidPrice = bidPrice;
}

Quote &Quote::withBidPrice(double bidPrice) noexcept {
    setBidPrice(bidPrice);

    return *this;
}

double Quote::getBidSize() const noexcept {
    return data_.bidSize;
}

void Quote::setBidSize(double bidSize) noexcept {
    data_.bidSize = bidSize;
}

Quote &Quote::withBidSize(double bidSize) noexcept {
    setBidSize(bidSize);

    return *this;
}

std::int64_t Quote::getAskTime() const noexcept {
    return data_.askTime;
}

void Quote::setAskTime(std::int64_t askTime) noexcept {
    data_.askTime = askTime;

    recomputeTimeMillisPart();
}

Quote &Quote::withAskTime(std::int64_t askTime) noexcept {
    setAskTime(askTime);

    return *this;
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

Quote &Quote::withAskExchangeCode(char askExchangeCode) noexcept {
    setAskExchangeCode(askExchangeCode);

    return *this;
}

void Quote::setAskExchangeCode(std::int16_t askExchangeCode) noexcept {
    data_.askExchangeCode = askExchangeCode;
}

Quote &Quote::withAskExchangeCode(std::int16_t askExchangeCode) noexcept {
    setAskExchangeCode(askExchangeCode);

    return *this;
}

double Quote::getAskPrice() const {
    return data_.askPrice;
}

void Quote::setAskPrice(double askPrice) {
    data_.askPrice = askPrice;
}

Quote &Quote::withAskPrice(double askPrice) noexcept {
    setAskPrice(askPrice);

    return *this;
}

double Quote::getAskSize() const {
    return data_.askSize;
}

void Quote::setAskSize(double askSize) {
    data_.askSize = askSize;
}

Quote &Quote::withAskSize(double askSize) noexcept {
    setAskSize(askSize);

    return *this;
}

void Quote::recomputeTimeMillisPart() noexcept {
    data_.timeMillisSequence = orOp(
        sal(time_util::getMillisFromTime(std::max(data_.askTime, data_.bidTime)), MILLISECONDS_SHIFT), getSequence());
}

void Quote::fillData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    MarketEvent::fillData(graalNative);

    const auto graalQuote = static_cast<dxfg_quote_t *>(graalNative);

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

    const auto graalQuote = static_cast<dxfg_quote_t *>(graalNative);

    graalQuote->market_event.event_type.clazz = DXFG_EVENT_QUOTE;
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

std::shared_ptr<Quote> Quote::fromGraal(void *graalNative) {
    if (!graalNative) {
        throw InvalidArgumentException("Unable to create Quote. The `graalNative` parameter is nullptr");
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_QUOTE) {
        throw InvalidArgumentException(fmt::format("Unable to create Quote. Wrong event class {}! Expected: {}.",
                                                   std::to_string(static_cast<dxfg_event_type_t *>(graalNative)->clazz),
                                                   std::to_string(DXFG_EVENT_QUOTE)));
    }

    auto quote = std::make_shared<Quote>();

    quote->fillData(graalNative);

    return quote;
}

void *Quote::toGraal() const {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug(toString() + "::toGraal()");
    }

    auto *graalQuote = new dxfg_quote_t{};

    fillGraalData(graalQuote);

    return graalQuote;
}

void Quote::freeGraal(void *graalNative) {
    if (!graalNative) {
        return;
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_QUOTE) {
        throw InvalidArgumentException(fmt::format(
            "Unable to free Quote's Graal data. Wrong event class {}! Expected: {}.",
            std::to_string(static_cast<dxfg_event_type_t *>(graalNative)->clazz), std::to_string(DXFG_EVENT_QUOTE)));
    }

    const auto graalQuote = static_cast<dxfg_quote_t *>(graalNative);

    freeGraalData(graalNative);

    delete graalQuote;
}

void Quote::assign(std::shared_ptr<EventType> event) {
    MarketEvent::assign(event);

    if (const auto other = event->sharedAs<Quote>(); other) {
        data_ = other->data_;
    }
}

Quote::Quote() noexcept {
}

Quote::Quote(const StringLike &eventSymbol) noexcept : MarketEvent(eventSymbol) {
}

Quote &Quote::withEventSymbol(const StringLike &eventSymbol) noexcept {
    MarketEvent::setEventSymbol(eventSymbol);

    return *this;
}

Quote &Quote::withEventTime(std::int64_t eventTime) noexcept {
    MarketEvent::setEventTime(eventTime);

    return *this;
}

std::int32_t Quote::getSequence() const noexcept {
    return andOp(data_.timeMillisSequence, MAX_SEQUENCE);
}

void Quote::setSequence(std::int32_t sequence) {
    assert(sequence >= 0 && static_cast<std::uint32_t>(sequence) <= MAX_SEQUENCE);

    if (sequence < 0 || static_cast<std::uint32_t>(sequence) > MAX_SEQUENCE) {
        throw InvalidArgumentException("Invalid sequence value = " + std::to_string(sequence));
    }

    data_.timeMillisSequence = orOp(andOp(data_.timeMillisSequence, ~MAX_SEQUENCE), sequence);
}

Quote &Quote::withSequence(std::int32_t sequence) noexcept {
    setSequence(sequence);

    return *this;
}

std::int64_t Quote::getTime() const noexcept {
    return math::floorDiv(std::max(data_.bidTime, data_.askTime), 1000LL) * 1000LL +
           shr(data_.timeMillisSequence, MILLISECONDS_SHIFT);
}

std::int64_t Quote::getTimeNanos() const noexcept {
    return time_nanos_util::getNanosFromMillisAndNanoPart(getTime(), data_.timeNanoPart);
}

std::int32_t Quote::getTimeNanoPart() const noexcept {
    return data_.timeNanoPart;
}

void Quote::setTimeNanoPart(std::int32_t timeNanoPart) noexcept {
    data_.timeNanoPart = timeNanoPart;
}

Quote &Quote::withTimeNanoPart(std::int32_t timeNanoPart) noexcept {
    setTimeNanoPart(timeNanoPart);

    return *this;
}

std::int64_t Quote::getBidTime() const noexcept {
    return data_.bidTime;
}

void Quote::setBidTime(std::int64_t bidTime) noexcept {
    data_.bidTime = bidTime;

    recomputeTimeMillisPart();
}

Quote &Quote::withBidTime(std::int64_t bidTime) noexcept {
    setBidTime(bidTime);

    return *this;
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

DXFCPP_END_NAMESPACE