// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>
#include "dxfeed_graal_cpp_api/event/market/Order.hpp"

namespace dxfcpp {

const EventTypeEnum &Order::TYPE = EventTypeEnum::ORDER;

void Order::fillData(void *graalNative) noexcept {
    OrderBase::fillData(graalNative);

    auto graalOrder = bit_cast<dxfg_order_t *>(graalNative);

    orderData_ = {dxfcpp::toString(graalOrder->market_maker)};
}

std::shared_ptr<Order> Order::fromGraal(void *graalNative) noexcept {
    if (!graalNative) {
        return {};
    }

    if (bit_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_ORDER) {
        return {};
    }

    try {
        auto order = std::make_shared<Order>();

        order->fillData(graalNative);

        return order;
    } catch (...) {
        // TODO: error handling
        return {};
    }
}

std::string Order::toString() const noexcept {
    return fmt::format("Order{{{}, marketMaker={}}}", baseFieldsToString(), getMarketMaker());
}

void *Order::toGraal() const noexcept {
    return nullptr;
}

void Order::freeGraal(void *graalNative) noexcept {
    if (!graalNative) {
        return;
    }

    auto eventType = bit_cast<dxfg_event_type_t *>(graalNative);

    if (eventType->clazz != DXFG_EVENT_ORDER) {
        return;
    }

    auto graalOrder = bit_cast<dxfg_order_t *>(graalNative);

    delete[] graalOrder->order_base.market_event.event_symbol;
    delete[] graalOrder->market_maker;

    delete graalOrder;
}

} // namespace dxfcpp