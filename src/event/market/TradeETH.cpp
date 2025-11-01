// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/market/TradeETH.hpp"

#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../../../include/dxfeed_graal_cpp_api/internal/TimeFormat.hpp"

#include <dxfg_api.h>
#include <fmt/format.h>
#include <string>

DXFCPP_BEGIN_NAMESPACE

const EventTypeEnum &TradeETH::TYPE = EventTypeEnum::TRADE_ETH;

void TradeETH::fillData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    TradeBase::fillData(graalNative);
}

void TradeETH::fillGraalData(void *graalNative) const noexcept {
    if (graalNative == nullptr) {
        return;
    }

    TradeBase::fillGraalData(graalNative);

    const auto graalTradeEth = static_cast<dxfg_trade_eth_t *>(graalNative);

    graalTradeEth->trade_base.market_event.event_type.clazz = DXFG_EVENT_TRADE_ETH;
}

std::shared_ptr<TradeETH> TradeETH::fromGraal(void *graalNative) {
    if (!graalNative) {
        throw InvalidArgumentException("Unable to create TradeETH. The `graalNative` parameter is nullptr");
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_TRADE_ETH) {
        throw InvalidArgumentException(fmt::format("Unable to create TradeETH. Wrong event class {}! Expected: {}.",
                                                   std::to_string(static_cast<dxfg_event_type_t *>(graalNative)->clazz),
                                                   std::to_string(DXFG_EVENT_TRADE_ETH)));
    }

    auto tradeEth = std::make_shared<TradeETH>();

    tradeEth->fillData(graalNative);

    return tradeEth;
}

std::string TradeETH::toString() const {
    return fmt::format("TradeETH{{{}}}", baseFieldsToString());
}

void *TradeETH::toGraal() const {
    if constexpr (Debugger::isDebug) {
        Debugger::debug(toString() + "::toGraal()");
    }

    auto *graalTradeEth = new dxfg_trade_eth_t{};

    fillGraalData(graalTradeEth);

    return graalTradeEth;
}

void TradeETH::freeGraal(void *graalNative) {
    if (!graalNative) {
        return;
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_TRADE_ETH) {
        throw InvalidArgumentException(
            fmt::format("Unable to free TradeETH's Graal data. Wrong event class {}! Expected: {}.",
                        std::to_string(static_cast<dxfg_event_type_t *>(graalNative)->clazz),
                        std::to_string(DXFG_EVENT_TRADE_ETH)));
    }

    const auto graalTradeEth = static_cast<dxfg_trade_eth_t *>(graalNative);

    freeGraalData(graalNative);

    delete graalTradeEth;
}

void TradeETH::assign(std::shared_ptr<EventType> event) {
    TradeBase::assign(event);

    // if (const auto other = event->sharedAs<TradeETH>(); other) {
    // }
}

DXFCPP_END_NAMESPACE