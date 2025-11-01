// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/market/Trade.hpp"

#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../../../include/dxfeed_graal_cpp_api/internal/TimeFormat.hpp"

#include <dxfg_api.h>
#include <fmt/format.h>
#include <string>

DXFCPP_BEGIN_NAMESPACE

const EventTypeEnum &Trade::TYPE = EventTypeEnum::TRADE;

void Trade::fillData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    TradeBase::fillData(graalNative);
}

void Trade::fillGraalData(void *graalNative) const noexcept {
    if (graalNative == nullptr) {
        return;
    }

    TradeBase::fillGraalData(graalNative);

    const auto graalTrade = static_cast<dxfg_trade_t *>(graalNative);

    graalTrade->trade_base.market_event.event_type.clazz = DXFG_EVENT_TRADE;
}

std::shared_ptr<Trade> Trade::fromGraal(void *graalNative) {
    if (!graalNative) {
        throw InvalidArgumentException("Unable to create Trade. The `graalNative` parameter is nullptr");
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_TRADE) {
        throw InvalidArgumentException(fmt::format("Unable to create Trade. Wrong event class {}! Expected: {}.",
                                                   std::to_string(static_cast<dxfg_event_type_t *>(graalNative)->clazz),
                                                   std::to_string(DXFG_EVENT_TRADE)));
    }

    auto trade = std::make_shared<Trade>();

    trade->fillData(graalNative);

    return trade;
}

std::string Trade::toString() const {
    return fmt::format("Trade{{{}}}", baseFieldsToString());
}

void *Trade::toGraal() const {
    if constexpr (Debugger::isDebug) {
        Debugger::debug(toString() + "::toGraal()");
    }

    auto *graalTrade = new dxfg_trade_t{};

    fillGraalData(graalTrade);

    return graalTrade;
}

void Trade::freeGraal(void *graalNative) {
    if (!graalNative) {
        return;
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_TRADE) {
        throw InvalidArgumentException(fmt::format(
            "Unable to free Trade's Graal data. Wrong event class {}! Expected: {}.",
            std::to_string(static_cast<dxfg_event_type_t *>(graalNative)->clazz), std::to_string(DXFG_EVENT_TRADE)));
    }

    const auto graalTrade = static_cast<dxfg_trade_t *>(graalNative);

    freeGraalData(graalNative);

    delete graalTrade;
}

void Trade::assign(std::shared_ptr<EventType> event) {
    TradeBase::assign(event);

    // if (const auto other = event->sharedAs<Trade>(); other) {
    // }
}

DXFCPP_END_NAMESPACE