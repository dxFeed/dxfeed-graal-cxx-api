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

void TimeAndSale::setExchangeCode(char exchangeCode) { data_.exchangeCode = utf8to16(exchangeCode); }

const EventTypeEnum &TimeAndSale::TYPE = EventTypeEnum::TIME_AND_SALE;

std::shared_ptr<TimeAndSale> TimeAndSale::fromGraalNative(void *graalNative) noexcept {
    if (!graalNative) {
        return {};
    }

    auto eventType = bit_cast<dxfg_event_type_t *>(graalNative);

    if (eventType->clazz != DXFG_EVENT_TIME_AND_SALE) {
        return {};
    }

    try {
        auto graalTimeAndSale = bit_cast<dxfg_time_and_sale_t *>(graalNative);
        auto timeAndSale = std::make_shared<TimeAndSale>(dxfcpp::toString(graalTimeAndSale->market_event.event_symbol));

        timeAndSale->setEventTime(graalTimeAndSale->market_event.event_time);
        timeAndSale->data_ = {
            graalTimeAndSale->event_flags,
            graalTimeAndSale->index,
            graalTimeAndSale->time_nano_part,
            graalTimeAndSale->exchange_code,
            graalTimeAndSale->price,
            graalTimeAndSale->size,
            graalTimeAndSale->bid_price,
            graalTimeAndSale->ask_price,
            dxfcpp::toString(graalTimeAndSale->exchange_sale_conditions),
            graalTimeAndSale->flags,
            dxfcpp::toString(graalTimeAndSale->buyer),
            dxfcpp::toString(graalTimeAndSale->seller),
        };

        return timeAndSale;
    } catch (...) {
        // TODO: error handling
        return {};
    }
}

std::string TimeAndSale::toString() const noexcept {
    return fmt::format("TimeAndSale{{{}, eventTime={}, eventFlags={:#x}, time={}, timeNanoPart={}, sequence={}, "
                       "exchange={}, price={}, size={}, bid={}, "
                       "ask={}, ESC='{}', TTE={}, side={}, spread={}, ETH={}, validTick={}, type={}{}{}}}",
                       MarketEvent::getEventSymbol(), formatTimeStampWithMillis(MarketEvent::getEventTime()),
                       getEventFlags().getMask(), formatTimeStampWithMillis(getTime()), getTimeNanoPart(),
                       getSequence(), encodeChar(getExchangeCode()), getPrice(), getSize(), getBidPrice(),
                       getAskPrice(), getExchangeSaleConditions(), encodeChar(getTradeThroughExempt()),
                       getAggressorSide().toString(), isSpreadLeg(), isExtendedTradingHours(), isValidTick(),
                       getType().toString(),
                       getBuyer().empty() ? std::string{} : fmt::format(", buyer='{}'", getBuyer()),
                       getSeller().empty() ? std::string{} : fmt::format(", seller='{}'", getSeller()));
}

} // namespace dxfcpp