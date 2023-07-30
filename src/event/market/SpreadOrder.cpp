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

const EventTypeEnum &SpreadOrder::TYPE = EventTypeEnum::SPREAD_ORDER;

void SpreadOrder::fillData(void *graalNative) noexcept {
    OrderBase::fillData(graalNative);

    auto graalSpreadOrder = bit_cast<dxfg_spread_order_t *>(graalNative);

    spreadOrderData_ = {dxfcpp::toString(graalSpreadOrder->spread_symbol)};
}

std::shared_ptr<SpreadOrder> SpreadOrder::fromGraalNative(void *graalNative) noexcept {
    if (!graalNative) {
        return {};
    }

    if (bit_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_SPREAD_ORDER) {
        return {};
    }

    try {
        auto spreadOrder = std::make_shared<SpreadOrder>();

        spreadOrder->fillData(graalNative);

        return spreadOrder;
    } catch (...) {
        // TODO: error handling
        return {};
    }
}

std::string SpreadOrder::toString() const noexcept {
    return fmt::format("SpreadOrder{{{}, spreadSymbol={}}}", baseFieldsToString(), getSpreadSymbol());
}

} // namespace dxfcpp