// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/market/TradeBase.hpp"

#include "../../../include/dxfeed_graal_cpp_api/event/market/Direction.hpp"
#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../../../include/dxfeed_graal_cpp_api/internal/TimeFormat.hpp"

#include <dxfg_api.h>
#include <fmt/format.h>
#include <string>

DXFCPP_BEGIN_NAMESPACE

void TradeBase::fillData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    MarketEvent::fillData(graalNative);

    const auto graalTradeBase = static_cast<dxfg_trade_base_t *>(graalNative);

    tradeBaseData_ = {
        .timeSequence = graalTradeBase->time_sequence,
        .timeNanoPart = graalTradeBase->time_nano_part,
        .exchangeCode = graalTradeBase->exchange_code,
        .price = graalTradeBase->price,
        .change = graalTradeBase->change,
        .size = graalTradeBase->size,
        .dayId = graalTradeBase->day_id,
        .dayVolume = graalTradeBase->day_volume,
        .dayTurnover = graalTradeBase->day_turnover,
        .flags = graalTradeBase->flags,
    };
}

void TradeBase::fillGraalData(void *graalNative) const noexcept {
    if (graalNative == nullptr) {
        return;
    }

    MarketEvent::fillGraalData(graalNative);

    const auto graalTradeBase = static_cast<dxfg_trade_base_t *>(graalNative);

    graalTradeBase->time_sequence = tradeBaseData_.timeSequence;
    graalTradeBase->time_nano_part = tradeBaseData_.timeNanoPart;
    graalTradeBase->exchange_code = tradeBaseData_.exchangeCode;
    graalTradeBase->price = tradeBaseData_.price;
    graalTradeBase->change = tradeBaseData_.change;
    graalTradeBase->size = tradeBaseData_.size;
    graalTradeBase->day_id = tradeBaseData_.dayId;
    graalTradeBase->day_volume = tradeBaseData_.dayVolume;
    graalTradeBase->day_turnover = tradeBaseData_.dayTurnover;
    graalTradeBase->flags = tradeBaseData_.flags;
}

void TradeBase::assign(std::shared_ptr<EventType> event) {
    MarketEvent::assign(event);

    if (const auto other = event->sharedAs<TradeBase>(); other) {
        tradeBaseData_ = other->tradeBaseData_;
    }
}

TradeBase::TradeBase() noexcept {
}

TradeBase::TradeBase(const StringLike &eventSymbol) noexcept : MarketEvent(eventSymbol) {
}

std::int64_t TradeBase::getTimeSequence() const noexcept {
    return tradeBaseData_.timeSequence;
}

void TradeBase::setTimeSequence(std::int64_t timeSequence) noexcept {
    tradeBaseData_.timeSequence = timeSequence;
}

std::int64_t TradeBase::getTime() const noexcept {
    return sar(tradeBaseData_.timeSequence, SECONDS_SHIFT) * 1000 +
           andOp(sar(tradeBaseData_.timeSequence, MILLISECONDS_SHIFT), MILLISECONDS_MASK);
}

void TradeBase::setTime(std::int64_t time) noexcept {
    // UB-less calculation of `(time.seconds << SECONDS_SHIFT) | (time.millis << MILLISECONDS_SHIFT) | sequence`
    tradeBaseData_.timeSequence =
        orOp(orOp(sal(static_cast<std::int64_t>(time_util::getSecondsFromTime(time)), SECONDS_SHIFT),
                  sal(static_cast<std::int64_t>(time_util::getMillisFromTime(time)), MILLISECONDS_SHIFT)),
             getSequence());
}

std::int64_t TradeBase::getTimeNanos() const noexcept {
    return time_nanos_util::getNanosFromMillisAndNanoPart(getTime(), tradeBaseData_.timeNanoPart);
}

void TradeBase::setTimeNanos(std::int64_t timeNanos) noexcept {
    setTime(time_nanos_util::getMillisFromNanos(timeNanos));
    tradeBaseData_.timeNanoPart = time_nanos_util::getNanoPartFromNanos(timeNanos);
}

void TradeBase::setTimeNanoPart(std::int32_t timeNanoPart) noexcept {
    tradeBaseData_.timeNanoPart = timeNanoPart;
}

std::int32_t TradeBase::getTimeNanoPart() const noexcept {
    return tradeBaseData_.timeNanoPart;
}

std::int32_t TradeBase::getSequence() const noexcept {
    return static_cast<std::int32_t>(andOp(tradeBaseData_.timeSequence, MAX_SEQUENCE));
}

void TradeBase::setSequence(std::int32_t sequence) {
    assert(sequence >= 0 && static_cast<std::uint32_t>(sequence) <= MAX_SEQUENCE);

    if (sequence < 0 || static_cast<std::uint32_t>(sequence) > MAX_SEQUENCE) {
        throw InvalidArgumentException("Invalid sequence value = " + std::to_string(sequence));
    }

    tradeBaseData_.timeSequence = orOp(andOp(tradeBaseData_.timeSequence, ~MAX_SEQUENCE), sequence);
}

std::int16_t TradeBase::getExchangeCode() const noexcept {
    return tradeBaseData_.exchangeCode;
}

std::string TradeBase::getExchangeCodeString() const noexcept {
    // TODO: cache [EN-8231]

    return utf16toUtf8String(tradeBaseData_.exchangeCode);
}

void TradeBase::setExchangeCode(char exchangeCode) noexcept {
    tradeBaseData_.exchangeCode = utf8to16(exchangeCode);
}

void TradeBase::setExchangeCode(std::int16_t exchangeCode) noexcept {
    tradeBaseData_.exchangeCode = exchangeCode;
}

double TradeBase::getPrice() const noexcept {
    return tradeBaseData_.price;
}

void TradeBase::setPrice(double price) noexcept {
    tradeBaseData_.price = price;
}

double TradeBase::getSize() const noexcept {
    return tradeBaseData_.size;
}

void TradeBase::setSize(double size) noexcept {
    tradeBaseData_.size = size;
}

std::int32_t TradeBase::getDayId() const noexcept {
    return tradeBaseData_.dayId;
}

void TradeBase::setDayId(std::int32_t dayId) noexcept {
    tradeBaseData_.dayId = dayId;
}

double TradeBase::getDayVolume() const noexcept {
    return tradeBaseData_.dayVolume;
}

void TradeBase::setDayVolume(double dayVolume) noexcept {
    tradeBaseData_.dayVolume = dayVolume;
}

double TradeBase::getDayTurnover() const noexcept {
    return tradeBaseData_.dayTurnover;
}

void TradeBase::setDayTurnover(double dayTurnover) noexcept {
    tradeBaseData_.dayTurnover = dayTurnover;
}

const Direction &TradeBase::getTickDirection() const & noexcept {
    return Direction::valueOf(getBits(tradeBaseData_.flags, DIRECTION_MASK, DIRECTION_SHIFT));
}

void TradeBase::setTickDirection(const Direction &direction) noexcept {
    tradeBaseData_.flags = setBits(tradeBaseData_.flags, DIRECTION_MASK, DIRECTION_SHIFT, direction.getCode());
}

bool TradeBase::isExtendedTradingHours() const noexcept {
    return andOp(tradeBaseData_.flags, ETH) != 0;
}

void TradeBase::setExtendedTradingHours(bool extendedTradingHours) noexcept {
    tradeBaseData_.flags = extendedTradingHours ? orOp(tradeBaseData_.flags, ETH) : andOp(tradeBaseData_.flags, ~ETH);
}

double TradeBase::getChange() const noexcept {
    return tradeBaseData_.change;
}

void TradeBase::setChange(double change) noexcept {
    tradeBaseData_.change = change;
}

std::string TradeBase::baseFieldsToString() const {
    return fmt::format(
        "{}, eventTime={}, time={}, timeNanoPart={}, sequence={}, exchange={}, price={}, "
        "change={}, size={}, day={}, dayVolume={}, dayTurnover={}, "
        "direction={}, ETH={}",
        MarketEvent::getEventSymbol(), TimeFormat::DEFAULT_WITH_MILLIS.format(MarketEvent::getEventTime()),
        TimeFormat::DEFAULT_WITH_MILLIS.format(getTime()), getTimeNanoPart(), getSequence(),
        encodeChar(getExchangeCode()), dxfcpp::toString(getPrice()), dxfcpp::toString(getChange()),
        dxfcpp::toString(getSize()), day_util::getYearMonthDayByDayId(getDayId()), dxfcpp::toString(getDayVolume()),
        dxfcpp::toString(getDayTurnover()), getTickDirection().toString(), isExtendedTradingHours());
}

DXFCPP_END_NAMESPACE