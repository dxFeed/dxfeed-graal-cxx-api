// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

namespace dxfcpp {

void TradeBase::fillData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    MarketEvent::fillData(graalNative);

    auto graalTradeBase = static_cast<dxfg_trade_base_t *>(graalNative);

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

    auto graalTradeBase = static_cast<dxfg_trade_base_t *>(graalNative);

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

std::string TradeBase::baseFieldsToString() const noexcept {
    return fmt::format("{}, eventTime={}, time={}, timeNanoPart={}, sequence={}, exchange={}, price={}, "
                       "change={}, size={}, day={}, dayVolume={}, dayTurnover={}, "
                       "direction={}, ETH={}",
                       MarketEvent::getEventSymbol(), TimeFormat::DEFAULT_WITH_MILLIS.format(MarketEvent::getEventTime()),
                       TimeFormat::DEFAULT_WITH_MILLIS.format(getTime()), getTimeNanoPart(), getSequence(),
                       encodeChar(getExchangeCode()), dxfcpp::toString(getPrice()), dxfcpp::toString(getChange()),
                       dxfcpp::toString(getSize()), day_util::getYearMonthDayByDayId(getDayId()),
                       dxfcpp::toString(getDayVolume()), dxfcpp::toString(getDayTurnover()),
                       getTickDirection().toString(), isExtendedTradingHours());
}

} // namespace dxfcpp