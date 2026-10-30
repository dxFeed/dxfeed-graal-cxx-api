// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/market/MarketMaker.hpp"

#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../../../include/dxfeed_graal_cpp_api/internal/TimeFormat.hpp"
#include "../../../include/dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp"
#include "../../internal/utils/JavaUtils.hpp"
#include "../../internal/utils/StringConstants.hpp"

#include <algorithm>
#include <dxfg_api.h>
#include <fmt/format.h>
#include <iterator>
#include <memory>
#include <string>
#include <utf8.h>
#include <utility>

DXFCPP_BEGIN_NAMESPACE

namespace {

// The QDS ShortString encoding (Java EventUtil.encodeShortString): up to 8 characters up to U+00FF, one byte each, the
// last character in the lowest byte; zero characters are skipped.
std::int64_t encodeShortString(const std::string &string) {
    std::u32string codePoints{};

    try {
        utf8::utf8to32(string.begin(), string.end(), std::back_inserter(codePoints));
    } catch (const utf8::exception &) {
        throw InvalidArgumentException("Invalid market maker: the string is not valid UTF-8");
    }

    if (codePoints.size() > 8) {
        throw InvalidArgumentException("Invalid market maker: the string is too long: " + string);
    }

    std::uint64_t code = 0;

    for (const auto c : codePoints) {
        if (c > 0xFF) {
            throw InvalidArgumentException("Invalid market maker: the string contains an illegal character: " + string);
        }

        // Each character takes one byte: the bytes already written move one byte up and the new character goes into
        // the lowest byte. So "ABC" becomes 0x0000000000414243: the first character ends up in the highest occupied
        // byte, and the unused high bytes stay zero.
        if (c != 0) {
            code = (code << 8U) | c;
        }
    }

    return static_cast<std::int64_t>(code);
}

// Java EventUtil.decodeShortString: zero code is null, zero bytes are skipped; the characters are converted to UTF-8.
std::optional<std::string> decodeShortString(std::int64_t code) {
    if (code == 0) {
        return std::nullopt;
    }

    std::string result{};

    // Reads the 8 bytes from the highest (bits 63..56, shift 56 = 64 - 8) down to the lowest (bits 7..0, shift 0), so
    // the characters come out in the order they were written. The zero bytes in front of a short string are skipped.
    // Java collects the bytes from the lowest one into a reversed code and reads it back; the result is the same.
    for (int shift = 56; shift >= 0; shift -= 8) {
        if (const auto c = static_cast<char32_t>((static_cast<std::uint64_t>(code) >> shift) & 0xFFU); c != 0) {
            utf8::append(c, std::back_inserter(result));
        }
    }

    return result;
}

const std::string &orNull(const std::optional<std::string> &value) {
    static const std::string NULL_STRING{"null"};

    return value ? *value : NULL_STRING;
}

} // namespace

const EventTypeEnum &MarketMaker::TYPE = EventTypeEnum::MARKET_MAKER;

void MarketMaker::fillData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    MarketEvent::fillData(graalNative);

    const auto graalMarketMaker = static_cast<dxfg_market_maker_t *>(graalNative);

    data_ = {
        .eventFlags = graalMarketMaker->event_flags,
        .index = graalMarketMaker->index,
        .bidTime = graalMarketMaker->bid_time,
        .bidPrice = graalMarketMaker->bid_price,
        .bidSize = graalMarketMaker->bid_size,
        .bidCount = graalMarketMaker->bid_count,
        .askTime = graalMarketMaker->ask_time,
        .askPrice = graalMarketMaker->ask_price,
        .askSize = graalMarketMaker->ask_size,
        .askCount = graalMarketMaker->ask_count,
    };
}

void MarketMaker::fillGraalData(void *graalNative) const noexcept {
    if (graalNative == nullptr) {
        return;
    }

    MarketEvent::fillGraalData(graalNative);

    const auto graalMarketMaker = static_cast<dxfg_market_maker_t *>(graalNative);

    graalMarketMaker->market_event.event_type.clazz = DXFG_EVENT_MARKET_MAKER;
    graalMarketMaker->event_flags = data_.eventFlags;
    graalMarketMaker->index = data_.index;
    graalMarketMaker->bid_time = data_.bidTime;
    graalMarketMaker->bid_price = data_.bidPrice;
    graalMarketMaker->bid_size = data_.bidSize;
    graalMarketMaker->bid_count = data_.bidCount;
    graalMarketMaker->ask_time = data_.askTime;
    graalMarketMaker->ask_price = data_.askPrice;
    graalMarketMaker->ask_size = data_.askSize;
    graalMarketMaker->ask_count = data_.askCount;
}

void MarketMaker::freeGraalData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    MarketEvent::freeGraalData(graalNative);
}

std::shared_ptr<MarketMaker> MarketMaker::fromGraal(void *graalNative) {
    if (!graalNative) {
        throw InvalidArgumentException("Unable to create MarketMaker. The `graalNative` parameter is nullptr");
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_MARKET_MAKER) {
        const auto eventType = static_cast<dxfg_event_type_t *>(graalNative)->clazz;

        throw InvalidArgumentException(fmt::format(
            "Unable to create MarketMaker. Wrong event class {}({})! Expected: {}", isolated::toString(eventType),
            std::to_string(eventType), isolated::toString(DXFG_EVENT_MARKET_MAKER)));
    }

    auto marketMaker = std::make_shared<MarketMaker>();

    marketMaker->fillData(graalNative);

    return marketMaker;
}

void *MarketMaker::toGraal() const {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug(toString() + "::toGraal()");
    }

    auto *graalMarketMaker = new dxfg_market_maker_t{};

    fillGraalData(graalMarketMaker);

    return graalMarketMaker;
}

void MarketMaker::freeGraal(void *graalNative) {
    if (!graalNative) {
        return;
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_MARKET_MAKER) {
        const auto eventType = static_cast<dxfg_event_type_t *>(graalNative)->clazz;

        throw InvalidArgumentException(fmt::format(
            "Unable to free MarketMaker's Graal data. Wrong event class {}({})! Expected: {}",
            isolated::toString(eventType), std::to_string(eventType), isolated::toString(DXFG_EVENT_MARKET_MAKER)));
    }

    const auto graalMarketMaker = static_cast<dxfg_market_maker_t *>(graalNative);

    freeGraalData(graalNative);

    delete graalMarketMaker;
}

void MarketMaker::assign(std::shared_ptr<EventType> event) {
    MarketEvent::assign(event);

    if (const auto other = event->sharedAs<MarketMaker>(); other) {
        data_ = other->data_;
    }
}

MarketMaker::MarketMaker() noexcept {
}

MarketMaker::MarketMaker(const StringLike &eventSymbol) noexcept : MarketEvent(eventSymbol) {
}

MarketMaker &MarketMaker::withEventSymbol(const StringLike &eventSymbol) noexcept {
    MarketEvent::setEventSymbol(eventSymbol);

    return *this;
}

MarketMaker &MarketMaker::withEventTime(std::int64_t eventTime) noexcept {
    MarketEvent::setEventTime(eventTime);

    return *this;
}

const IndexedEventSource &MarketMaker::getSource() const & noexcept {
    return IndexedEventSource::DEFAULT;
}

std::int32_t MarketMaker::getEventFlags() const noexcept {
    return data_.eventFlags;
}

EventFlagsMask MarketMaker::getEventFlagsMask() const noexcept {
    return EventFlagsMask(data_.eventFlags);
}

void MarketMaker::setEventFlags(std::int32_t eventFlags) noexcept {
    data_.eventFlags = eventFlags;
}

MarketMaker &MarketMaker::withEventFlags(std::int32_t eventFlags) noexcept {
    MarketMaker::setEventFlags(eventFlags);

    return *this;
}

void MarketMaker::setEventFlags(const EventFlagsMask &eventFlags) noexcept {
    data_.eventFlags = static_cast<std::int32_t>(eventFlags.getMask());
}

MarketMaker &MarketMaker::withEventFlags(const EventFlagsMask &eventFlags) noexcept {
    MarketMaker::setEventFlags(eventFlags);

    return *this;
}

std::int64_t MarketMaker::getIndex() const noexcept {
    return data_.index;
}

void MarketMaker::setIndex(std::int64_t index) {
    if (index < 0) {
        throw InvalidArgumentException("Negative index: " + std::to_string(index));
    }

    data_.index = index;
}

MarketMaker &MarketMaker::withIndex(std::int64_t index) {
    MarketMaker::setIndex(index);

    return *this;
}

std::int64_t MarketMaker::getTime() const noexcept {
    return std::max(data_.bidTime, data_.askTime);
}

std::int16_t MarketMaker::getExchangeCode() const noexcept {
    return static_cast<std::int16_t>(getBits(data_.index, EXCHANGE_MASK, EXCHANGE_SHIFT));
}

std::string MarketMaker::getExchangeCodeString() const noexcept {
    return utf16toUtf8String(getExchangeCode());
}

void MarketMaker::setExchangeCode(char exchangeCode) noexcept {
    setExchangeCode(utf8to16(exchangeCode));
}

MarketMaker &MarketMaker::withExchangeCode(char exchangeCode) noexcept {
    setExchangeCode(exchangeCode);

    return *this;
}

void MarketMaker::setExchangeCode(std::int16_t exchangeCode) noexcept {
    // The UTF-16 unit goes to bits 47..32: EXCHANGE_MASK keeps its 16 bits and leaves the MMID bits untouched.
    data_.index = setBits(data_.index, EXCHANGE_MASK, EXCHANGE_SHIFT, static_cast<std::uint16_t>(exchangeCode));
}

MarketMaker &MarketMaker::withExchangeCode(std::int16_t exchangeCode) noexcept {
    setExchangeCode(exchangeCode);

    return *this;
}

std::string MarketMaker::getMarketMaker() const {
    auto marketMaker = getMarketMakerOpt();

    if (!marketMaker) {
        return nulString();
    }

    return std::move(*marketMaker);
}

std::optional<std::string> MarketMaker::getMarketMakerOpt() const {
    return decodeShortString(getBits(data_.index, MMID_MASK, MMID_SHIFT));
}

void MarketMaker::setMarketMaker(const StringLike &marketMaker) {
    // The short string code has up to 8 bytes, but MMID_MASK keeps only the lowest 4 (bits 31..0), i.e. the last 4
    // characters, as in Java: "ABCDEF" is stored as "CDEF". The exchange code bits are left untouched.
    data_.index = setBits(data_.index, MMID_MASK, MMID_SHIFT, encodeShortString(std::string(marketMaker)));
}

MarketMaker &MarketMaker::withMarketMaker(const StringLike &marketMaker) {
    setMarketMaker(marketMaker);

    return *this;
}

std::int64_t MarketMaker::getBidTime() const noexcept {
    return data_.bidTime;
}

void MarketMaker::setBidTime(std::int64_t bidTime) noexcept {
    data_.bidTime = bidTime;
}

MarketMaker &MarketMaker::withBidTime(std::int64_t bidTime) noexcept {
    setBidTime(bidTime);

    return *this;
}

double MarketMaker::getBidPrice() const noexcept {
    return data_.bidPrice;
}

void MarketMaker::setBidPrice(double bidPrice) noexcept {
    data_.bidPrice = bidPrice;
}

MarketMaker &MarketMaker::withBidPrice(double bidPrice) noexcept {
    setBidPrice(bidPrice);

    return *this;
}

double MarketMaker::getBidSize() const noexcept {
    return data_.bidSize;
}

void MarketMaker::setBidSize(double bidSize) noexcept {
    data_.bidSize = bidSize;
}

MarketMaker &MarketMaker::withBidSize(double bidSize) noexcept {
    setBidSize(bidSize);

    return *this;
}

std::int64_t MarketMaker::getBidCount() const noexcept {
    return data_.bidCount;
}

void MarketMaker::setBidCount(std::int64_t bidCount) noexcept {
    data_.bidCount = bidCount;
}

MarketMaker &MarketMaker::withBidCount(std::int64_t bidCount) noexcept {
    setBidCount(bidCount);

    return *this;
}

std::int64_t MarketMaker::getAskTime() const noexcept {
    return data_.askTime;
}

void MarketMaker::setAskTime(std::int64_t askTime) noexcept {
    data_.askTime = askTime;
}

MarketMaker &MarketMaker::withAskTime(std::int64_t askTime) noexcept {
    setAskTime(askTime);

    return *this;
}

double MarketMaker::getAskPrice() const noexcept {
    return data_.askPrice;
}

void MarketMaker::setAskPrice(double askPrice) noexcept {
    data_.askPrice = askPrice;
}

MarketMaker &MarketMaker::withAskPrice(double askPrice) noexcept {
    setAskPrice(askPrice);

    return *this;
}

double MarketMaker::getAskSize() const noexcept {
    return data_.askSize;
}

void MarketMaker::setAskSize(double askSize) noexcept {
    data_.askSize = askSize;
}

MarketMaker &MarketMaker::withAskSize(double askSize) noexcept {
    setAskSize(askSize);

    return *this;
}

std::int64_t MarketMaker::getAskCount() const noexcept {
    return data_.askCount;
}

void MarketMaker::setAskCount(std::int64_t askCount) noexcept {
    data_.askCount = askCount;
}

MarketMaker &MarketMaker::withAskCount(std::int64_t askCount) noexcept {
    setAskCount(askCount);

    return *this;
}

std::string MarketMaker::toString() const {
    return fmt::format(
        "MarketMaker{{{}, eventTime={}, source={}, eventFlags={:#x}, index={:#x}, time={}, exchange={}, "
        "marketMaker='{}', bidTime={}, bidPrice={}, bidSize={}, bidCount={}, askTime={}, askPrice={}, "
        "askSize={}, askCount={}}}",
        MarketEvent::getEventSymbol(), TimeFormat::DEFAULT_WITH_MILLIS.format(MarketEvent::getEventTime()),
        getSource().toString(), getEventFlagsMask().getMask(), static_cast<std::uint64_t>(getIndex()),
        TimeFormat::DEFAULT_WITH_MILLIS.format(getTime()), encodeChar(getExchangeCode()), orNull(getMarketMakerOpt()),
        TimeFormat::DEFAULT_WITH_MILLIS.format(getBidTime()), java::doubleToString(getBidPrice()),
        java::doubleToString(getBidSize()), getBidCount(), TimeFormat::DEFAULT_WITH_MILLIS.format(getAskTime()),
        java::doubleToString(getAskPrice()), java::doubleToString(getAskSize()), getAskCount());
}

DXFCPP_END_NAMESPACE
