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

void TradeBase::setSequence(std::int32_t sequence) {
    assert(sequence >= 0 && static_cast<std::uint32_t>(sequence) <= MAX_SEQUENCE);

    if (sequence < 0 || static_cast<std::uint32_t>(sequence) > MAX_SEQUENCE) {
        throw InvalidArgumentException("Invalid sequence value = " + std::to_string(sequence));
    }

    tradeBaseData_.timeSequence = orOp(andOp(tradeBaseData_.timeSequence, ~MAX_SEQUENCE), sequence);
}

const Direction &TradeBase::getTickDirection() const & noexcept {
    return Direction::valueOf(getBits(tradeBaseData_.flags, DIRECTION_MASK, DIRECTION_SHIFT));
}

void TradeBase::setTickDirection(const Direction &direction) noexcept {
    tradeBaseData_.flags = setBits(tradeBaseData_.flags, DIRECTION_MASK, DIRECTION_SHIFT, direction.getCode());
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