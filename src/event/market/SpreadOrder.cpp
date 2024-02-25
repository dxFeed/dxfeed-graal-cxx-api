// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include "dxfeed_graal_cpp_api/event/market/SpreadOrder.hpp"
#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

namespace dxfcpp {

const EventTypeEnum &SpreadOrder::TYPE = EventTypeEnum::SPREAD_ORDER;

void SpreadOrder::fillData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    OrderBase::fillData(graalNative);

    auto graalSpreadOrder = static_cast<dxfg_spread_order_t *>(graalNative);

    spreadOrderData_ = {.spreadSymbol = dxfcpp::toStringOpt(graalSpreadOrder->spread_symbol)};
}

void SpreadOrder::fillGraalData(void *graalNative) const noexcept {
    if (graalNative == nullptr) {
        return;
    }

    OrderBase::fillGraalData(graalNative);

    auto graalSpreadOrder = static_cast<dxfg_spread_order_t *>(graalNative);

    graalSpreadOrder->order_base.market_event.event_type.clazz = dxfg_event_clazz_t::DXFG_EVENT_SPREAD_ORDER;
    graalSpreadOrder->spread_symbol = createCString(spreadOrderData_.spreadSymbol);
}

void SpreadOrder::freeGraalData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    MarketEvent::freeGraalData(graalNative);

    auto graalSpreadOrder = static_cast<dxfg_spread_order_t *>(graalNative);

    delete[] graalSpreadOrder->spread_symbol;
}

std::shared_ptr<SpreadOrder> SpreadOrder::fromGraal(void *graalNative) noexcept {
    if (!graalNative) {
        return {};
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_SPREAD_ORDER) {
        return {};
    }

    try {
        auto spreadOrder = std::make_shared<SpreadOrder>();

        spreadOrder->fillData(graalNative);

        return spreadOrder;
    } catch (...) {
        // TODO: error handling [EN-8232]
        return {};
    }
}

std::string SpreadOrder::toString() const noexcept {
    return fmt::format("SpreadOrder{{{}, spreadSymbol={}}}", baseFieldsToString(), getSpreadSymbol());
}

void *SpreadOrder::toGraal() const noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug(toString() + "::toGraal()");
    }

    auto *graalSpreadOrder = new (std::nothrow) dxfg_spread_order_t{};

    if (!graalSpreadOrder) {
        // TODO: error handling [EN-8232]

        return nullptr;
    }

    fillGraalData(static_cast<void *>(graalSpreadOrder));

    return static_cast<void *>(graalSpreadOrder);
}

void SpreadOrder::freeGraal(void *graalNative) noexcept {
    if (!graalNative) {
        return;
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_SPREAD_ORDER) {
        return;
    }

    auto graalSpreadOrder = static_cast<dxfg_spread_order_t *>(graalNative);

    freeGraalData(graalNative);

    delete graalSpreadOrder;
}

} // namespace dxfcpp