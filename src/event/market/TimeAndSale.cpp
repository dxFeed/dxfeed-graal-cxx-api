// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/market/TimeAndSale.hpp"

#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../../../include/dxfeed_graal_cpp_api/internal/TimeFormat.hpp"

#include <dxfg_api.h>
#include <fmt/format.h>
#include <string>

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

    const auto graalTimeAndSale = static_cast<dxfg_time_and_sale_t *>(graalNative);

    data_ = {
        .eventFlags = graalTimeAndSale->event_flags,
        .index = graalTimeAndSale->index,
        .timeNanoPart = graalTimeAndSale->time_nano_part,
        .exchangeCode = graalTimeAndSale->exchange_code,
        .price = graalTimeAndSale->price,
        .size = graalTimeAndSale->size,
        .bidPrice = graalTimeAndSale->bid_price,
        .askPrice = graalTimeAndSale->ask_price,
        .exchangeSaleConditions = toStringOpt(graalTimeAndSale->exchange_sale_conditions),
        .flags = graalTimeAndSale->flags,
        .buyer = toStringOpt(graalTimeAndSale->buyer),
        .seller = toStringOpt(graalTimeAndSale->seller),
    };
}

void TimeAndSale::fillGraalData(void *graalNative) const noexcept {
    if (graalNative == nullptr) {
        return;
    }

    MarketEvent::fillGraalData(graalNative);

    const auto graalTimeAndSale = static_cast<dxfg_time_and_sale_t *>(graalNative);

    graalTimeAndSale->market_event.event_type.clazz = DXFG_EVENT_TIME_AND_SALE;
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

    const auto graalTimeAndSale = static_cast<dxfg_time_and_sale_t *>(graalNative);

    delete[] graalTimeAndSale->exchange_sale_conditions;
    delete[] graalTimeAndSale->buyer;
    delete[] graalTimeAndSale->seller;
}

std::shared_ptr<TimeAndSale> TimeAndSale::fromGraal(void *graalNative) {
    if (!graalNative) {
        throw InvalidArgumentException("Unable to create TimeAndSale. The `graalNative` parameter is nullptr");
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_TIME_AND_SALE) {
        throw InvalidArgumentException(fmt::format("Unable to create TimeAndSale. Wrong event class {}! Expected: {}.",
                                                   std::to_string(static_cast<dxfg_event_type_t *>(graalNative)->clazz),
                                                   std::to_string(DXFG_EVENT_TIME_AND_SALE)));
    }

    auto timeAndSale = std::make_shared<TimeAndSale>();

    timeAndSale->fillData(graalNative);

    return timeAndSale;
}

std::string TimeAndSale::toString() const {
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
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug(toString() + "::toGraal()");
    }

    auto *graalTimeAndSale = new dxfg_time_and_sale_t{};

    fillGraalData(graalTimeAndSale);

    return graalTimeAndSale;
}

void TimeAndSale::freeGraal(void *graalNative) {
    if (!graalNative) {
        return;
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_TIME_AND_SALE) {
        throw InvalidArgumentException(
            fmt::format("Unable to free TimeAndSale's Graal data. Wrong event class {}! Expected: {}.",
                        std::to_string(static_cast<dxfg_event_type_t *>(graalNative)->clazz),
                        std::to_string(DXFG_EVENT_TIME_AND_SALE)));
    }

    const auto graalTimeAndSale = static_cast<dxfg_time_and_sale_t *>(graalNative);

    freeGraalData(graalNative);

    delete graalTimeAndSale;
}

void TimeAndSale::assign(std::shared_ptr<EventType> event) {
    MarketEvent::assign(event);

    if (const auto other = event->sharedAs<TimeAndSale>(); other) {
        data_ = other->data_;
    }
}

DXFCPP_END_NAMESPACE