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

    auto graalTrade = static_cast<dxfg_trade_t *>(graalNative);

    graalTrade->trade_base.market_event.event_type.clazz = dxfg_event_clazz_t::DXFG_EVENT_TRADE;
}

std::shared_ptr<Trade> Trade::fromGraal(void *graalNative) noexcept {
    if (!graalNative) {
        return {};
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != dxfg_event_clazz_t::DXFG_EVENT_TRADE) {
        return {};
    }

    try {
        auto trade = std::make_shared<Trade>();

        trade->fillData(graalNative);

        return trade;
    } catch (...) {
        // TODO: error handling [EN-8232]
        return {};
    }
}

std::string Trade::toString() const noexcept {
    return fmt::format("Trade{{{}}}", baseFieldsToString());
}

void *Trade::toGraal() const {
    if constexpr (Debugger::isDebug) {
        Debugger::debug(toString() + "::toGraal()");
    }

    auto *graalTrade = new dxfg_trade_t{};

    fillGraalData(static_cast<void *>(graalTrade));

    return static_cast<void *>(graalTrade);
}

void Trade::freeGraal(void *graalNative) noexcept {
    if (!graalNative) {
        return;
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != dxfg_event_clazz_t::DXFG_EVENT_TRADE) {
        return;
    }

    auto graalTrade = static_cast<dxfg_trade_t *>(graalNative);

    MarketEvent::freeGraalData(graalNative);

    delete graalTrade;
}

DXFCPP_END_NAMESPACE