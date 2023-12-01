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
}

std::shared_ptr<TradeETH> TradeETH::fromGraal(void *graalNative) noexcept {
    if (!graalNative) {
        return {};
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != dxfg_event_clazz_t::DXFG_EVENT_TRADE_ETH) {
        return {};
    }

    try {
        auto tradeEth = std::make_shared<TradeETH>();

        tradeEth->fillData(graalNative);

        return tradeEth;
    } catch (...) {
        // TODO: error handling [EN-8232]
        return {};
    }
}

std::string TradeETH::toString() const noexcept {
    return fmt::format("TradeETH{{{}}}", baseFieldsToString());
}

void *TradeETH::toGraal() const noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug(toString() + "::toGraal()");
    }

    auto *graalTradeEth = new (std::nothrow) dxfg_trade_eth_t{
        .trade_base = {.market_event = {.event_type = {.clazz = dxfg_event_clazz_t::DXFG_EVENT_TRADE_ETH}}}};

    if (!graalTradeEth) {
        // TODO: error handling [EN-8232]

        return nullptr;
    }

    fillGraalData(static_cast<void *>(graalTradeEth));

    return static_cast<void *>(graalTradeEth);
}

void TradeETH::freeGraal(void *graalNative) noexcept {
    if (!graalNative) {
        return;
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != dxfg_event_clazz_t::DXFG_EVENT_TRADE_ETH) {
        return;
    }

    auto graalTradeEth = static_cast<dxfg_trade_eth_t *>(graalNative);

    MarketEvent::freeGraalData(graalNative);

    delete graalTradeEth;
}

} // namespace dxfcpp