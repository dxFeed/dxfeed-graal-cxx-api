// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include "dxfeed_graal_cpp_api/event/market/Order.hpp"
#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

DXFCPP_BEGIN_NAMESPACE

const EventTypeEnum &Order::TYPE = EventTypeEnum::ORDER;

void Order::fillData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    OrderBase::fillData(graalNative);

    auto graalOrder = static_cast<dxfg_order_t *>(graalNative);

    orderData_ = {.marketMaker = dxfcpp::toStringOpt(graalOrder->market_maker)};
}

void Order::fillGraalData(void *graalNative) const noexcept {
    if (graalNative == nullptr) {
        return;
    }

    OrderBase::fillGraalData(graalNative);

    auto graalOrder = static_cast<dxfg_order_t *>(graalNative);

    graalOrder->order_base.market_event.event_type.clazz = dxfg_event_clazz_t::DXFG_EVENT_ORDER;
    graalOrder->market_maker = createCString(orderData_.marketMaker);
}

void Order::freeGraalData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    MarketEvent::freeGraalData(graalNative);

    auto graalOrder = static_cast<dxfg_order_t *>(graalNative);

    delete[] graalOrder->market_maker;
}

std::shared_ptr<Order> Order::fromGraal(void *graalNative) {
    if (!graalNative) {
        throw InvalidArgumentException("Unable to create Order. The `graalNative` parameter is nullptr");
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != dxfg_event_clazz_t::DXFG_EVENT_ORDER) {
        throw InvalidArgumentException(
            fmt::format("Unable to create Order. Wrong event class {}! Expected: {}.",
                        std::to_string(static_cast<int>(static_cast<dxfg_event_type_t *>(graalNative)->clazz)),
                        std::to_string(static_cast<int>(dxfg_event_clazz_t::DXFG_EVENT_ORDER))));
    }

    auto order = std::make_shared<Order>();

    order->fillData(graalNative);

    return order;
}

std::string Order::toString() const {
    return fmt::format("Order{{{}, marketMaker={}}}", baseFieldsToString(), getMarketMaker());
}

void *Order::toGraal() const {
    if constexpr (Debugger::isDebug) {
        Debugger::debug(toString() + "::toGraal()");
    }

    auto *graalOrder = new dxfg_order_t{};

    fillGraalData(static_cast<void *>(graalOrder));

    return static_cast<void *>(graalOrder);
}

void Order::freeGraal(void *graalNative) {
    if (!graalNative) {
        return;
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != dxfg_event_clazz_t::DXFG_EVENT_ORDER) {
        throw InvalidArgumentException(
            fmt::format("Unable to free Order's Graal data. Wrong event class {}! Expected: {}.",
                        std::to_string(static_cast<int>(static_cast<dxfg_event_type_t *>(graalNative)->clazz)),
                        std::to_string(static_cast<int>(dxfg_event_clazz_t::DXFG_EVENT_ORDER))));
    }

    auto graalOrder = static_cast<dxfg_order_t *>(graalNative);

    freeGraalData(graalNative);

    delete graalOrder;
}

void Order::assign(std::shared_ptr<EventType> event) {
    OrderBase::assign(event);

    if (const auto other = event->sharedAs<Order>(); other) {
        orderData_ = other->orderData_;
    }
}

DXFCPP_END_NAMESPACE