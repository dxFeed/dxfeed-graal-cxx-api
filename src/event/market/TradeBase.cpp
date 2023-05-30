// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <cstring>
#include <memory>
#include <utf8.h>
#include <utility>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

namespace dxfcpp {

void TradeBase::setExchangeCode(char exchangeCode) { data_.exchangeCode = utf8to16(exchangeCode); }

std::string TradeBase::baseFieldsToString() const noexcept {
    return fmt::format("{}, eventTime={}, time={}, timeNanoPart={}, sequence={}, exchange={}, price={}, "
                       "change={}, size={}, day={}, dayVolume={}, dayTurnover={}, "
                       "direction={}, ETH={}",
                       MarketEvent::getEventSymbol(), formatTimeStampWithMillis(MarketEvent::getEventTime()),
                       formatTimeStampWithMillis(getTime()), getTimeNanoPart(), getSequence(),
                       encodeChar(getExchangeCode()), getPrice(), getChange(), getSize(),
                       day_util::getYearMonthDayByDayId(getDayId()), getDayVolume(), getDayTurnover(),
                       getTickDirection().toString(), isExtendedTradingHours());
}

} // namespace dxfcpp