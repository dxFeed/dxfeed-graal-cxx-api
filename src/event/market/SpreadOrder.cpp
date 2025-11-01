// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/market/SpreadOrder.hpp"

#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../../../include/dxfeed_graal_cpp_api/internal/TimeFormat.hpp"

#include <dxfg_api.h>
#include <fmt/format.h>
#include <string>

DXFCPP_BEGIN_NAMESPACE

const EventTypeEnum &SpreadOrder::TYPE = EventTypeEnum::SPREAD_ORDER;

void SpreadOrder::fillData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    OrderBase::fillData(graalNative);

    const auto graalSpreadOrder = static_cast<dxfg_spread_order_t *>(graalNative);

    spreadOrderData_ = {.spreadSymbol = toStringOpt(graalSpreadOrder->spread_symbol)};
}

void SpreadOrder::fillGraalData(void *graalNative) const noexcept {
    if (graalNative == nullptr) {
        return;
    }

    OrderBase::fillGraalData(graalNative);

    const auto graalSpreadOrder = static_cast<dxfg_spread_order_t *>(graalNative);

    graalSpreadOrder->order_base.market_event.event_type.clazz = DXFG_EVENT_SPREAD_ORDER;
    graalSpreadOrder->spread_symbol = createCString(spreadOrderData_.spreadSymbol);
}

void SpreadOrder::freeGraalData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    MarketEvent::freeGraalData(graalNative);

    const auto graalSpreadOrder = static_cast<dxfg_spread_order_t *>(graalNative);

    delete[] graalSpreadOrder->spread_symbol;
}

std::shared_ptr<SpreadOrder> SpreadOrder::fromGraal(void *graalNative) {
    if (!graalNative) {
        throw InvalidArgumentException("Unable to create SpreadOrder. The `graalNative` parameter is nullptr");
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_SPREAD_ORDER) {
        throw InvalidArgumentException(fmt::format("Unable to create SpreadOrder. Wrong event class {}! Expected: {}.",
                                                   std::to_string(static_cast<dxfg_event_type_t *>(graalNative)->clazz),
                                                   std::to_string(DXFG_EVENT_SPREAD_ORDER)));
    }

    auto spreadOrder = std::make_shared<SpreadOrder>();

    spreadOrder->fillData(graalNative);

    return spreadOrder;
}

std::string SpreadOrder::toString() const {
    return fmt::format("SpreadOrder{{{}, spreadSymbol={}}}", baseFieldsToString(), getSpreadSymbol());
}

void *SpreadOrder::toGraal() const {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug(toString() + "::toGraal()");
    }

    auto *graalSpreadOrder = new dxfg_spread_order_t{};

    fillGraalData(graalSpreadOrder);

    return graalSpreadOrder;
}

void SpreadOrder::freeGraal(void *graalNative) {
    if (!graalNative) {
        return;
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_SPREAD_ORDER) {
        throw InvalidArgumentException(
            fmt::format("Unable to free SpreadOrder's Graal data. Wrong event class {}! Expected: {}.",
                        std::to_string(static_cast<dxfg_event_type_t *>(graalNative)->clazz),
                        std::to_string(DXFG_EVENT_SPREAD_ORDER)));
    }

    const auto graalSpreadOrder = static_cast<dxfg_spread_order_t *>(graalNative);

    freeGraalData(graalNative);

    delete graalSpreadOrder;
}

void SpreadOrder::assign(std::shared_ptr<EventType> event) {
    OrderBase::assign(event);

    if (const auto other = event->sharedAs<SpreadOrder>(); other) {
        spreadOrderData_ = other->spreadOrderData_;
    }
}

DXFCPP_END_NAMESPACE