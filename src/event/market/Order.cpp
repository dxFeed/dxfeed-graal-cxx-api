// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

namespace dxfcpp {

const EventTypeEnum &Order::TYPE = EventTypeEnum::ORDER;

void Order::fillData(void *graalNative) noexcept {
    OrderBase::fillData(graalNative);

    auto graalOrder = bit_cast<dxfg_order_t *>(graalNative);

    orderData_ = {dxfcpp::toString(graalOrder->market_maker)};
}

std::shared_ptr<Order> Order::fromGraalNative(void *graalNative) noexcept {
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

} // namespace dxfcpp