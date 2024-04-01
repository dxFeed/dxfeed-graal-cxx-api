// Copyright (c) 2024 Devexperts LLC.
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

DXFCPP_BEGIN_NAMESPACE

void TimeAndSale::setExchangeCode(char exchangeCode) noexcept {
    data_.exchangeCode = utf8to16(exchangeCode);
}

const EventTypeEnum &TimeAndSale::TYPE = EventTypeEnum::TIME_AND_SALE;

void TimeAndSale::fillData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    MarketEvent::fillData(graalNative);

    auto graalTimeAndSale = static_cast<dxfg_time_and_sale_t *>(graalNative);

    data_ = {
        .eventFlags = graalTimeAndSale->event_flags,
        .index = graalTimeAndSale->index,
        .timeNanoPart = graalTimeAndSale->time_nano_part,
        .exchangeCode = graalTimeAndSale->exchange_code,
        .price = graalTimeAndSale->price,
        .size = graalTimeAndSale->size,
        .bidPrice = graalTimeAndSale->bid_price,
        .askPrice = graalTimeAndSale->ask_price,
        .exchangeSaleConditions = dxfcpp::toStringOpt(graalTimeAndSale->exchange_sale_conditions),
        .flags = graalTimeAndSale->flags,
        .buyer = dxfcpp::toStringOpt(graalTimeAndSale->buyer),
        .seller = dxfcpp::toStringOpt(graalTimeAndSale->seller),
    };
}

void TimeAndSale::fillGraalData(void *graalNative) const noexcept {
    if (graalNative == nullptr) {
        return;
    }

    MarketEvent::fillGraalData(graalNative);

    auto graalTimeAndSale = static_cast<dxfg_time_and_sale_t *>(graalNative);

    graalTimeAndSale->market_event.event_type.clazz = dxfg_event_clazz_t::DXFG_EVENT_TIME_AND_SALE;
    graalTimeAndSale->event_flags = data_.eventFlags;
    graalTimeAndSale->index = data_.index;
    graalTimeAndSale->time_nano_part = data_.timeNanoPart;
    graalTimeAndSale->exchange_code = data_.exchangeCode;
    graalTimeAndSale->price = data_.price;
    graalTimeAndSale->size = data_.size;
    graalTimeAndSale->bid_price = data_.bidPrice;
    graalTimeAndSale->ask_price = data_.askPrice;
    graalTimeAndSale->exchange_sale_conditions = createCString(data_.exchangeSaleConditions);
    graalTimeAndSale->flags = data_.flags;
    graalTimeAndSale->buyer = createCString(data_.buyer);
    graalTimeAndSale->seller = createCString(data_.seller);
}

void TimeAndSale::freeGraalData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    MarketEvent::freeGraalData(graalNative);

    auto graalTimeAndSale = static_cast<dxfg_time_and_sale_t *>(graalNative);

    delete[] graalTimeAndSale->exchange_sale_conditions;
    delete[] graalTimeAndSale->buyer;
    delete[] graalTimeAndSale->seller;
}

std::shared_ptr<TimeAndSale> TimeAndSale::fromGraal(void *graalNative) {
    if (!graalNative) {
        return {};
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != dxfg_event_clazz_t::DXFG_EVENT_TIME_AND_SALE) {
        return {};
    }

    try {
        auto timeAndSale = std::make_shared<TimeAndSale>();

        timeAndSale->fillData(graalNative);

        return timeAndSale;
    } catch (...) {
        // TODO: error handling [EN-8232]
        return {};
    }
}

std::string TimeAndSale::toString() const noexcept {
    return fmt::format("TimeAndSale{{{}, eventTime={}, eventFlags={:#x}, time={}, timeNanoPart={}, sequence={}, "
                       "exchange={}, price={}, size={}, bid={}, "
                       "ask={}, ESC='{}', TTE={}, side={}, spread={}, ETH={}, validTick={}, type={}{}{}}}",
                       MarketEvent::getEventSymbol(),
                       TimeFormat::DEFAULT_WITH_MILLIS.format(MarketEvent::getEventTime()),
                       getEventFlagsMask().getMask(), TimeFormat::DEFAULT_WITH_MILLIS.format(getTime()),
                       getTimeNanoPart(), getSequence(), encodeChar(getExchangeCode()), dxfcpp::toString(getPrice()),
                       dxfcpp::toString(getSize()), dxfcpp::toString(getBidPrice()), dxfcpp::toString(getAskPrice()),
                       getExchangeSaleConditions(), encodeChar(getTradeThroughExempt()), getAggressorSide().toString(),
                       isSpreadLeg(), isExtendedTradingHours(), isValidTick(), getType().toString(),
                       getBuyer().empty() ? std::string{} : fmt::format(", buyer='{}'", getBuyer()),
                       getSeller().empty() ? std::string{} : fmt::format(", seller='{}'", getSeller()));
}

void *TimeAndSale::toGraal() const {
    if constexpr (Debugger::isDebug) {
        Debugger::debug(toString() + "::toGraal()");
    }

    auto *graalTimeAndSale = new dxfg_time_and_sale_t{};

    fillGraalData(static_cast<void *>(graalTimeAndSale));

    return static_cast<void *>(graalTimeAndSale);
}

void TimeAndSale::freeGraal(void *graalNative) {
    if (!graalNative) {
        return;
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != dxfg_event_clazz_t::DXFG_EVENT_TIME_AND_SALE) {
        return;
    }

    auto graalTimeAndSale = static_cast<dxfg_time_and_sale_t *>(graalNative);

    freeGraalData(graalNative);

    delete graalTimeAndSale;
}

DXFCPP_END_NAMESPACE